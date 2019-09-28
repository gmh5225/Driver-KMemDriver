#include "pch.h"
#include "KMemDriver.h"
#include "KInterface.h"
#include "DLLHelper.h"
#include "PatternScanner.h"

#include <array>
#include <iostream>
#include <iomanip>
#include <windows.h>
#include <Shlwapi.h>

#define WHEXOUT std::setfill(L'0') << std::setw(16) << std::hex

static BOOL running = false;
static const wchar_t *wName = L"HUNT";


static bool consoleHandler(int signal) {
	if (signal == CTRL_C_EVENT) {
		if (!running)
			exit(EXIT_FAILURE);
		running = false;
		std::wcout << L"Waiting for graceful shutdown .." << std::endl;
	}
	return true;
}

static void printBuf(UCHAR *buf, SIZE_T siz, SIZE_T bytesBeforeNewline) {
	unsigned int i, j;
	const unsigned char colors[] = { 10,11,12,13,14,15 };
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

	for (i = 0, j = 0; i < siz; ++i) {
		if (i % bytesBeforeNewline == 0) {
			SetConsoleTextAttribute(hConsole, colors[j++ % (sizeof colors)]);
			wprintf(L"\n0x%04X: ", i);
		}
		wprintf(L"%02X ", buf[i]);
	}
	wprintf(L"\n");
	SetConsoleTextAttribute(hConsole, 15);
}

static BOOL CALLBACK enumWindowsProc(HWND hWnd, LPARAM lParam)
{
	int length = GetWindowTextLength(hWnd);
	TCHAR* buffer;
	buffer = new TCHAR[length + 1];
	memset(buffer, 0, (length + 1) * sizeof(TCHAR));
	GetWindowText(hWnd, buffer, length + 1);
	if (!wcscmp(buffer, wName))
		*(HWND *)lParam = hWnd;
	delete[] buffer;
	return TRUE;
}

int wmain(int argc, wchar_t **argv)
{
	HANDLE targetPID = 0;
	PVOID buf;
	HANDLE kevent;
	HANDLE uevent;

	KInterface &ki = KInterface::getInstance();
	std::vector<MEMORY_BASIC_INFORMATION> pages;
	std::vector<MODULE_DATA> modules;

	std::wcout << L"Waiting for window title: '" << wName << L"'" << std::endl;

	HWND targetHWND = NULL;
	while (1) {
		if (!EnumWindows(enumWindowsProc, (LPARAM)&targetHWND)) {
			return 1;
		}
		if (targetHWND) {
			std::wcout << L"Found window '" << wName << L"' with Handle 0x"
				<< std::hex << targetHWND << std::endl;
			break;
		}
		Sleep(1000);
	}
	GetWindowThreadProcessId(targetHWND, (LPDWORD)&targetPID);

	SetConsoleCtrlHandler((PHANDLER_ROUTINE)consoleHandler, TRUE);

	if (!ki.Init()) {
		std::wcout << L"Kernel Interface Init() failed" << std::endl;
		return 1;
	}

	try {
		buf = ki.getBuffer();
		kevent = ki.getKHandle();
		uevent = ki.getUHandle();
	}
	catch (std::runtime_error& err) {
		std::wcout << err.what() << std::endl;
		return 1;
	}

	std::wcout << L"Buffer.: " << buf << std::endl;
	std::wcout << L"KHandle: " << kevent << std::endl;
	std::wcout << L"UHandle: " << uevent << std::endl;

	if (!ki.Handshake()) {
		std::wcout << L"Kernel Interface Handshake() failed" << std::endl;
		return 1;
	}
	if (targetPID) {
		if (!ki.Modules(targetPID, modules))
			std::wcout << L"Kernel Interface Modules() failed with 0x"
			<< std::hex << ki.getLastNtStatus() << std::endl;
		else std::wcout << L"Got " << std::dec << modules.size() << L" modules for pid 0x"
			<< std::hex << targetPID << std::endl;
		if (!ki.Pages(targetPID, pages))
			std::wcout << L"Kernel Interface Pages() failed with 0x"
			<< std::hex << ki.getLastNtStatus() << std::endl;
		else std::wcout << L"Got " << std::dec << pages.size() << L" mapped pages for pid 0x"
			<< std::hex << targetPID << std::endl;
	}

	running = TRUE;
	do {
		if (ki.RecvWait() == SRR_TIMEOUT) {
			std::wcout << L"Ping -> ";
			if (!ki.Ping()) {
				std::wcout << L"Got no valid PONG, abort!" << std::endl;
				running = FALSE;
			}
			else std::wcout << L"PONG!" << std::endl;
		}

		if (!running)
			break;

		try {
			if (targetPID) {
				for (MODULE_DATA& md : modules) {
					if (!strncmp(md.BaseDllName, "CrySystem.dll",
						sizeof md.BaseDllName))
					{
#if 0
						std::wcout << L"CrySystem.dll.......: 0x" << WHEXOUT << md.DllBase << std::endl;
						UINT32 tmp = 0xDEADBEEF;
						KMemory::Wpm<UINT32>(targetPID, (PVOID)((UINT64)md.DllBase + 0x566800), &tmp);
						std::wcout << L"RDATA...............: 0x" << WHEXOUT << KMemory::Rpm<UINT32>(targetPID, (PVOID)((UINT64)md.DllBase + 0x566800)) << std::endl;
						KMemory::Wpm<UINT32>(targetPID, (PVOID)((UINT64)md.DllBase + 0x1800), &tmp);
						std::wcout << L"TEXT................: 0x" << WHEXOUT << KMemory::Rpm<UINT32>(targetPID, (PVOID)((UINT64)md.DllBase + 0x1800)) << std::endl;
#endif
					}
					else
						if (!strncmp(md.BaseDllName, "CryEntitySystem.dll",
							sizeof md.BaseDllName))
						{
							/* "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Tools\MSVC\14.16.27023\bin\Hostx64\x64\cl.exe" /Zp2 /c /d1reportSingleClassLayoutCEntitySystem C:\Users\segfault\Source\Repos\CRYENGINE\Code\CryEngine\CryEntitySystem\EntitySystem.cpp /I C:\Users\segfault\Source\Repos\CRYENGINE\Code\CryEngine\CryCommon /I "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Tools\MSVC\14.16.27023\include" /I "C:\Program Files (x86)\Windows Kits\10\Include\10.0.17763.0\ucrt" /I "C:\Program Files (x86)\Windows Kits\10\Include\10.0.17763.0\shared" /I "C:\Program Files (x86)\Windows Kits\10\Include\10.0.17763.0\um" */
/*

class CEntitySystem     size(788880):
		+---
 0      | +--- (base class IEntitySystem)
 0      | | {vfptr}
		| +---
 8      | ?$array@V?$vector@PEAUIEntitySystemSink@@V?$allocator@PEAUIEntitySystemSink@@@std@@@std@@$03 m_sinks
104     | m_pISystem
112     | SEntityArray m_entityArray
786524  | ?$vector@IV?$allocator@I@std@@ m_staticEntityIds
786548  | ?$vector@PEAVCEntity@@V?$allocator@PEAVCEntity@@@std@@ m_deletedEntities
786572  | ?$vector@PEAVCEntity@@V?$allocator@PEAVCEntity@@@std@@ m_deferredUsedEntities
786596  | ?$multimap@PEBDIU?$less_stricmp@PEBD@stl@@V?$allocator@U?$pair@QEBDI@std@@@std@@ m_mapEntityNames
786612  | ?$CEntityComponentsVector@USMinimalEntityComponentRecord@@ m_updatedEntityComponents
786654  | ?$CEntityComponentsVector@USMinimalEntityComponentRecord@@ m_prePhysicsUpdatedEntityComponents
786696  | ?$multimap@VCTimeValue@@USEntityTimerEvent@@U?$less@VCTimeValue@@@std@@V?$STLPoolAllocator@U?$pair@$$CBVCTimeValue@@USEntityTimerEvent@@@std@@UPSyncNone@stl@@$0A@$0A@@stl@@ m_timersMap
786712  | ?$vector@USEntityTimerEvent@@V?$allocator@USEntityTimerEvent@@@std@@ m_currentTimers
786736  | m_bTimersPause
		| <alignment member> (size=1)
786738  | CTimeValue m_nStartPause
786746  | m_pEntityScriptBinding
786754  | m_pClassRegistry
786762  | m_pPhysicsEventListener
786770  | m_pReflectionRegistry
786778  | m_pAreaManager
786786  | m_pEntityLoadManager
786794  | ?$unordered_map@UCryGUID@@IU?$hash@UCryGUID@@@std@@U?$equal_to@UCryGUID@@@3@V?$allocator@U?$pair@$$CBUCryGUID@@I@std@@@3@ m_guidMap
786858  | ?$unordered_map@UCryGUID@@IU?$hash@UCryGUID@@@std@@U?$equal_to@UCryGUID@@@3@V?$allocator@U?$pair@$$CBUCryGUID@@I@std@@@3@ m_genIdMap
786922  | m_pBreakableManager
786930  | m_pEntityArchetypeManager
786938  | m_pGeomCacheAttachmentManager
786946  | m_pCharacterBoneAttachmentManager
786954  | m_pPartitionGrid
786962  | m_pProximityTriggerSystem
786970  | m_idForced
786974  | m_bLocked
786975  | m_bSupportLegacy64bitGuids
786976  | ?$map@V?$CryStringT@D@@PEAVCEntityLayer@@U?$less@V?$CryStringT@D@@@std@@V?$allocator@U?$pair@$$CBV?$CryStringT@D@@PEAVCEntityLayer@@@std@@@4@ m_layers
786992  | ?$vector@USEntityLayerGarbage@@V?$allocator@USEntityLayerGarbage@@@std@@ m_garbageLayerHeaps
787016  | ?$unique_ptr@VCEntityComponentsCache@@U?$default_delete@VCEntityComponentsCache@@@std@@ m_entityComponentsCache
787024  | ?$unique_ptr@VCEntityObjectDebugger@@U?$default_delete@VCEntityObjectDebugger@@@std@@ m_pEntityObjectDebugger
787032  | ?$vector@USLayerProfile@CEntitySystem@@V?$allocator@USLayerProfile@CEntitySystem@@@std@@ m_layerProfiles
787056  | ?$array@USProfiledEntityEvent@CEntitySystem@@$0DJ@ m_profiledEvents


class CEntitySystem::SEntityArray       size(786412):
		+---
 0      | +--- (base class SSaltBufferArray)
 0      | | ?$array@USSaltBufferElement@SSaltBufferArray@@$0PPPO@ m_buffer
262136. | | m_freeListStartIndex (bitstart=0,nbits=16)
262138. | | m_maxUsedEntityIndex (bitstart=0,nbits=16)
		| +---
262140  | ?$array@PEAVCEntity@@$0PPPO@ m_array


class SSaltBufferArray::SSaltBufferElement      size(4):
		+---
 0.     | m_salt (bitstart=0,nbits=16)
 2.     | m_nextIndex (bitstart=0,nbits=16)
		+---

class CEntity   size(412):
		+---
 0      | +--- (base class IEntity)
 0      | | {vfptr}
		| +---
 8      | ?$CEnumFlags@W4EInternalFlag@CEntity@@ m_internalFlags
12      | m_sendEventRecursionCount
		| <alignment member> (size=1)
14      | m_componentChangeState
16      | ?$CryStringT@D m_name
24      | m_pClass
32      | m_pArchetype
40      | STransformHierarchy m_hierarchy
82      | ?$_smart_ptr@UIMaterial@@ m_pMaterial
90      | m_pEntityLinks
98      | m_pGridLocation
106     | m_pProximityEntity
114     | ?$unique_ptr@USLegacySchematycData@CEntity@@U?$default_delete@USLegacySchematycData@CEntity@@@std@@ m_pLegacySchematycData
122     | ?$DynArray@V?$unique_ptr@USExternalEventListener@CEntity@@U?$default_delete@USExternalEventListener@CEntity@@@std@@@std@@HU?$SmallDynStorage@U?$AllocCompatible@UModuleAlloc@NAlloc@@@NAlloc@@@NArray@@ m_externalEventListeners
130     | ?$CEnumFlags@W4EEvent@Entity@Cry@@ m_eventListenerMask
138     | ?$DynArray@V?$unique_ptr@USEventListenerSet@CEntity@@U?$default_delete@USEventListenerSet@CEntity@@@std@@@std@@HU?$SmallDynStorage@U?$AllocCompatible@UModuleAlloc@NAlloc@@@NAlloc@@@NArray@@ m_simpleEventListeners
146     | ?$unique_ptr@UINetEntity@@U?$default_delete@UINetEntity@@@std@@ m_pNetEntity
154     | CEntityRender m_render
228     | CEntityPhysics m_physics
244     | CryGUID m_guid
260     | m_id
264     | m_aiObjectID
268     | m_flags
272     | m_flagsExtended
273     | EEntitySimulationMode m_simulationMode
274     | ?$Vec3_tpl@M m_position
286     | ?$Quat_tpl@M m_rotation
302     | ?$Vec3_tpl@M m_scale
		| <alignment member> (size=6)
320     | ?$Matrix34H@M m_worldTM
368     | m_keepAliveCounter
370     | ?$CEntityComponentsVector@USEntityComponentRecord@@ m_components
		+---

class Vec3_tpl<double>  size(24):
		+---
 0      | +--- (base class INumberVector<double,3,struct Vec3_tpl<double> >)
 0      | | +--- (base class INumberArray<double,3>)
		| | +---
		| +---
 0      | x
 8      | y
16      | z
		+---

class Vec3_tpl<float>   size(12):
		+---
 0      | +--- (base class INumberVector<float,3,struct Vec3_tpl<float> >)
 0      | | +--- (base class INumberArray<float,3>)
		| | +---
		| +---
 0      | x
 4      | y
 8      | z
		+---

*/
#if 1
/* Found: void CEntitySystem::LoadInternalState(IDataReadStream& reader) */
							UINT64 g_pEnv = KMemory::Rpm<UINT64>(targetPID,
								(PVOID)((UINT64)md.DllBase + 0x28C3F8));
							std::wcout << L"g_pEnv..............: 0x" << WHEXOUT << g_pEnv << std::endl;
#if 0
							// ?? ?? ?? ?? ?? ?? 85 C0 0F 84 5E 02 00 00
							BYTE aa[] = { 0x90, 0x90, 0x90, 0x90, 0x31, 0xC0 };
							KMemoryBuf::Wpm<sizeof aa>(targetPID, (PVOID)((UINT64)md.DllBase + 0x70619), &aa[0]);
							BYTE bb[sizeof aa] = {};
							KMemoryBuf::Rpm<sizeof aa>(targetPID, (PVOID)((UINT64)md.DllBase + 0x70619), &bb[0]);
							printBuf(bb, sizeof bb, 32);
#else
							static bool first = true;
							if (first) {
								first = false;
#if 0
								PVOID targetAddr = (PVOID)((UINT64)NULL);
								SIZE_T targetSize = 4096;
								if (!ki.VAlloc(targetPID, &targetAddr, &targetSize, PAGE_EXECUTE_READWRITE)) {
									std::wcout << L"VAlloc failed" << std::endl;
								}
								if (!ki.VUnlink(targetPID, targetAddr)) {
									std::wcout << L"VUnlink failed" << std::endl;
								}
#endif						
								SymbolResolver sresolv;
								DLLHelper dll(sresolv);
								if (!dll.Init(targetPID, "./TestDLL.dll")) {
									std::wcout << L"DLL Init failed" << std::endl;
								}
								if (!dll.VerifyHeader()) {
									std::wcout << L"DLL VerifyHeader failed" << std::endl;
								}
								if (!dll.InitTargetMemory()) {
									std::wcout << L"DLL InitTargetMemory failed" << std::endl;
								}
								if (!dll.HasImports())
								{
									std::wcout << L"DLL has no ImportTable" << std::endl;
								}
								else if (!dll.FixImports()) {
									std::wcout << L"DLL FixImports failed" << std::endl;
								}
								if (!dll.HasRelocs()) {
									std::wcout << L"DLL has no RelocTable" << std::endl;
								}
								else if (!dll.FixRelocs()) {
									std::wcout << L"DLL FixRelocs failed" << std::endl;
								}
								if (!dll.CopyHeaderAndSections()) {
									std::wcout << L"DLL CopyHeaderAndSections failed" << std::endl;
								}
								std::wcout << L"DLL mapping succesful, "
									<< "BaseAddress: " << WHEXOUT << dll.GetBaseAddress()
									<< ", EntryPoint: " << WHEXOUT << dll.GetEntryPoint() << std::endl;

								PVOID targetAddr = (PVOID)(dll.GetBaseAddress());
								std::wcout << "ADDRESS -> " << WHEXOUT << targetAddr << std::endl;
								if (!ki.VUnlink(targetPID, targetAddr)) {
									std::wcout << L"VUnlink failed" << std::endl;
								}

								UINT64 globalEnvAddr = 0;
								for (MODULE_DATA& md : modules) {
									if (!strncmp(md.BaseDllName, "CryAction.dll",
										sizeof md.BaseDllName)) {

										struct loadlib_user_data llua;
										char * cryDllDir = new char[sizeof md.FullDllPath];
										std::memcpy(cryDllDir, md.FullDllPath, sizeof md.FullDllPath);
										PathRemoveFileSpecA(cryDllDir);
										llua.additionalDllSearchDirectories.push_back(std::string(cryDllDir));
										delete cryDllDir;
										for (auto& dir : llua.additionalDllSearchDirectories) {
											std::wcout << L"AdditionalDLLDir: "
												<< std::wstring(dir.begin(), dir.end()) << std::endl;
										}

										PatternScanner pscan(sresolv, &map_loadlib, &llua);
										std::vector<SIZE_T> foundAddresses;
										pscan.Scan(md, "48 8B 48 20 48 8B 01 FF 90 20 01 00 00", foundAddresses);
										for (auto& addr : foundAddresses) {
											std::wcout << "Addr: " << addr << std::endl;
											BYTE content[32];
											KMemoryBuf::Rpm<sizeof content>(targetPID, (PVOID)addr, &content[0]);
											printBuf(content, sizeof content, 32);
										}

										// pEnv: 48 8B 48 20 48 8B 01 FF 90 20 01 00 00
										globalEnvAddr = (UINT64)md.DllBase + 0x70E848;
										break;
									}
								}

								BYTE cc[] = { /* push rax; push rbx; push rcx; push rdx; push rsi;
												 push rdi; push rsp; push rbp; push r8; push r9;
												 push r10; push r11; push r12; push r13; push r14;
												 push r15 */
											  0x50, 0x53, 0x51, 0x52, 0x56, 0x57,
											  0x54, 0x55, 0x41, 0x50, 0x41, 0x51,
											  0x41, 0x52, 0x41, 0x53, 0x41, 0x54,
											  0x41, 0x55, 0x41, 0x56, 0x41, 0x57,
											  /* nops */
											  0x90, 0x90, 0x90, 0x90, 0x90,
											  /* mov rcx, 0x0000000000000000 */
											  0x48, 0xB9,
											  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
											  /* mov rax, 0x0000000000000000 */
											  0x48, 0xB8,
											  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
											  /* call rax */
											  0xFF, 0xD0,
											  /* nops */
											  0x90, 0x90,
											  /* pop r15; pop r14; pop r13; pop r12; pop r11;
												 pop r10; pop r9; pop r8; pop rbp; pop rsp;
												 pop rdi; pop rsi; pop rdx; pop rcx; pop rbx;
												 pop rax */
											  0x41, 0x5F, 0x41, 0x5E, 0x41, 0x5D,
											  0x41, 0x5C, 0x41, 0x5B, 0x41, 0x5A,
											  0x41, 0x59, 0x41, 0x58, 0x5D, 0x5C,
											  0x5F, 0x5E, 0x5A, 0x59, 0x5B, 0x58,
											  /* nops */
											  0x90, 0x90,
											  /* mov rax, 0x0000000000000000 */
											  0x48, 0xB8,
											  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
											  /* jmp rax */
											  0xFF, 0xE0 };
								*(UINT64 *)((BYTE *)cc + 31) = globalEnvAddr;
								*(UINT64 *)((BYTE *)cc + 41) = dll.GetEntryPoint();
								/* PATTERN: 48 89 4C 24 08 48 83 EC 48 +275 */
								UINT64 jumpBackAddr = (UINT64)md.DllBase + 0x70885;
								*(UINT64 *)((BYTE *)cc + 81) = jumpBackAddr;
								printBuf(cc, sizeof cc, 32);
								KMemoryBuf::Wpm<sizeof cc>(targetPID, (PVOID)targetAddr, &cc[0]);

#if 1
								BYTE dd[] = { 0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xE0 };
								*(UINT64 *)((BYTE *)dd + 2) = (UINT64)targetAddr;
								printBuf(dd, sizeof dd, 32);
								/* PATTERN: 48 89 4C 24 08 48 83 EC 48 +9 */
								KMemoryBuf::Wpm<sizeof dd>(targetPID, (PVOID)((UINT64)md.DllBase + 0x70619), &dd[0]);
#endif
							}
#endif
#if 0
							UINT64 m_idForced = KMemory::Rpm<UINT64>(targetPID,
								(PVOID)((UINT64)g_pEnv + 786970));
							std::wcout << L"m_pidForced.........: 0x" << WHEXOUT << m_idForced << std::endl;

							UINT64 m_pISystem = KMemory::Rpm<UINT64>(targetPID,
								(PVOID)((UINT64)g_pEnv + 104));
							std::wcout << L"m_pISystem..........: 0x" << WHEXOUT << m_pISystem << std::endl;

							UINT16 m_freeListStartIndex = KMemory::Rpm<UINT16>(targetPID,
								(PVOID)((UINT64)g_pEnv + 112 + 262136));
							std::wcout << L"m_freeListStartIndex: 0x" << WHEXOUT << m_freeListStartIndex << std::endl;

							UINT16 m_maxUsedEntityIndex = KMemory::Rpm<UINT16>(targetPID,
								(PVOID)((UINT64)g_pEnv + 112 + 262138));
							std::wcout << L"m_maxUsedEntityIndex: 0x" << WHEXOUT << m_maxUsedEntityIndex << std::endl;
#endif
#if 0
							//UINT64 startOffsetMaxUsedEntities = (m_maxUsedEntityIndex < m_freeListStartIndex ? m_maxUsedEntityIndex : m_freeListStartIndex) * sizeof(PVOID);
							UINT64 startOffsetMaxUsedEntities = m_freeListStartIndex * sizeof(PVOID);
							//startOffsetMaxUsedEntities -= 50 * sizeof(PVOID);
							std::array<PVOID, 1024> entities;
							if (KInterface::getInstance().RPM(targetPID, (PVOID)((UINT64)g_pEnv + 112 + 262140 + 12 + startOffsetMaxUsedEntities), (BYTE*)&entities, sizeof entities, NULL)) {
								for (PVOID ent : entities) {
									if (ent == NULL) {
										continue;
									}

									const UINT64 additional_offset = 14;
									BYTE entity[412];
									//std::cout << "Got Entity: " << std::hex << ent << ", ";
									if (KInterface::getInstance().RPM(targetPID, ent, (BYTE*)&entity[0], sizeof entity, NULL)) {

										PVOID name_str = &entity[16 + additional_offset];
										UINT32 id = *(UINT32 *)&entity[260 + additional_offset];
										UINT32 flags = *(UINT32 *)&entity[268 + additional_offset];
										UINT8 extended = *(UINT8 *)&entity[272 + additional_offset];
										UINT16 keepAlive = *(UINT16 *)&entity[368 + additional_offset];
										float pos_x = *(float *)&entity[274 + additional_offset];
										float pos_y = *(float *)&entity[278 + additional_offset];
										float pos_z = *(float *)&entity[282 + additional_offset];

										if (keepAlive == 0 || id == 0) {
											continue;
										}

										//if ((flags & 0x2000 /* ENTITY_FLAG_HAS_AI */) == 0 && (flags & 0x8000 /* ENTITY_FLAG_CAMERA_SOURCE */) == 0) {
#if 1
										std::cout << "Name Ptr: " << std::hex << name_str
											<< ", id: " << std::setw(8) << std::hex << (UINT32)id
											<< ", flags: " << std::setw(8) << std::hex << (UINT32)flags
											<< ", extended: " << std::setw(8) << std::hex << (UINT32)extended
											<< ", keepAlive: " << std::setw(8) << std::hex << (UINT16)keepAlive
											<< ", pos_x: " << std::setw(8) << (float)pos_x
											<< ", pos_y: " << std::setw(8) << (float)pos_y
											<< ", pos_z: " << std::setw(8) << (float)pos_z
											<< std::endl;
#endif
										//}

										printBuf(entity, sizeof entity, 32);
										//break;
									}
									else std::wcerr << "Get Entity failed" << std::endl;
								}
							}
							else std::wcerr << "Get EntityArray failed" << std::endl;
#endif
#if 0
							BYTE tmp[sizeof SSystemGlobalEnvironment * 2] = { 0 };
							SSIZE_T siz = KMemoryBuf::Rpm<sizeof tmp>(targetPID, (PVOID)(g_pEnv), tmp);

							SSystemGlobalEnvironment *gEnv = (SSystemGlobalEnvironment *)&tmp[0];
							std::wcout << std::hex
								<< gEnv->p3DEngine << ", "
								<< gEnv->pEntitySystem << ", "
								<< gEnv->bMultiplayer << ", "
								<< gEnv->mMainThreadId << ", "
								<< gEnv->nMainFrameID << ", "
								<< gEnv->szDebugStatus << ", "
								<< gEnv->pMonoRuntime << ", "
								<< gEnv->callbackStartSection << ", "
								<< gEnv->callbackEndSection << ", "
								<< gEnv->bUnattendedMode << ", "
								<< gEnv->bTesting << ", "
								<< gEnv->bNoRandomSeed << ", "
								<< gEnv->bDeepProfiling << ", "
								<< std::endl;

							/* Found: void CEntitySystem::LoadInternalState(IDataReadStream& reader)
							 * search for String "ExistingEntity: '%s' '%s'"
							 */
							UINT64 m_EntityArray = KMemory::Rpm<UINT64>(targetPID,
								(PVOID)((UINT64)gEnv->pEntitySystem + 0x40078));
							//m_EntityArray += 0x40078;
							std::wcout << WHEXOUT
								<< m_EntityArray
								<< std::endl;

							USHORT nops;
							nops = KMemory::Rpm<USHORT>(targetPID,
								(PVOID)((UINT64)md.DllBase + 0x12C3C7));
							std::wcout << std::hex << "+++ " << nops << std::endl;
							nops = 0x9090;
							KMemory::Wpm<USHORT>(targetPID,
								(PVOID)((UINT64)md.DllBase + 0x12C3C7), &nops);
#if 0
							UINT64 tmp2[1024];
							SSIZE_T siz2 = KMemoryBuf::Rpm<sizeof tmp2>(targetPID, (PVOID)(m_EntityArray), (PBYTE)tmp2);
							SIZE_T i = 0;
							for (UINT64 val : tmp2) {
								if (val)
									std::wcout << WHEXOUT << val << ", ";
								if (++i % 16 == 0)
									std::wcout << std::endl;
							}
							/*
							if (siz2 > 0)
								printBuf(tmp2, sizeof tmp2, 64);
							*/
#endif
#endif
#if 0
							SSIZE_T siz3;
							static Diff<65535> diff = { 0 };
							siz3 = KScan::BinDiffSimple(targetPID, (PVOID)(m_EntityArray), &diff);
							if (siz3 > 0) {
								std::wcout << L"Got " << std::dec << diff.diffs.size() << L" diffs" << std::endl;
								for (auto& e : diff.diffs) {
									std::wcout << std::dec << L"0x"
										<< std::hex << (PVOID)(/*(ULONG_PTR)(m_EntityArray)+*/e.first)
										<< " - " << std::hex << e.second
										<< L": ";
									printBuf((UCHAR *)((ULONG_PTR)(diff.current_buffer) + e.first), e.second, e.second);
								}
						}
#endif
#if 0
#if 1
							//UCHAR scan[1024] = { 0 };
							UCHAR scan[] = "anti_alles";
							SSIZE_T found = KScan::ScanSimple<UCHAR, sizeof scan>(targetPID,
								(PVOID)(m_EntityArray), 65535, scan);
#else
							SSIZE_T found = (SSIZE_T)(pIEntitySystem)+8192;
#endif
							if (found >= 0) {
								std::wcout << "FOUND: 0x" << std::hex << (PVOID)found << std::endl;
								UCHAR tmp[4096] = { 0 };
								SSIZE_T siz = KMemoryBuf::Rpm<UCHAR, sizeof tmp>(targetPID, (PVOID)found, tmp);
								if (siz > 0) {
									printBuf(tmp, sizeof tmp, 64);
#if 0
									UINT64 i;
									for (i = 0; i < sizeof tmp; i += 8) {
										UINT64 value = *(UINT64 *)&tmp[i];
										if (value)
											printf("0x%p ", (PVOID)value);
									}
									printf("\nGot %llu entities ..\n", i);
#endif
								}
				}
#endif
#endif
						}
#if 0
						else if (!strncmp(md.BaseDllName, "CryRenderD3D11.dll",
							sizeof md.BaseDllName))
						{
							UINT32 displayWidth = KMemory::Rpm<UINT32>(targetPID,
								(PVOID)((ULONGLONG)md.DllBase + /* 0x19F0FE */ 0x5EA870));
							UINT32 displayHeight = KMemory::Rpm<UINT32>(targetPID,
								(PVOID)((ULONGLONG)md.DllBase + /* 0x19F0F0 */ 0x5EA9DC));
							std::wcout << L"Display.........: " << std::dec << displayWidth
								<< " x " << displayHeight << std::endl;
			}
#endif
#if 0
						else if (!strncmp(md.BaseDllName, "ntdll.dll",
							sizeof md.BaseDllName))
						{
							UCHAR tmp[8192] = { 0 };
							SSIZE_T siz = KMemoryBuf::Rpm<UCHAR, sizeof tmp>(targetPID, (PVOID)((ULONGLONG)md.DllBase), tmp);
							/*
													if (siz > 0)
														printBuf(tmp, sizeof tmp, 64);

													UCHAR scan[] = { 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC };
													SSIZE_T found = KScan::ScanSimple<UCHAR, sizeof scan>(targetPID,
														(PVOID)((ULONGLONG)md.DllBase), sizeof tmp, scan);
													std::wcout << "FOUND: 0x" << std::hex << (found - (ULONGLONG)md.DllBase) << std::endl;
							*/
							static Diff<10> diff = { 0 };
							siz = KScan::BinDiffSimple(targetPID, (PVOID)((ULONGLONG)md.DllBase), &diff);
							if (siz > 0) {
								std::wcout << L"Got " << std::dec << diff.diffs.size() << L" diffs" << std::endl;
								for (auto& e : diff.diffs) {
									printBuf((UCHAR *)((ULONGLONG)md.DllBase + e.first), e.second, e.second);
									/*
									std::wcout << std::dec << L"  addr: " << e.first
										<< std::endl << L"  size: " << e.second
										<< std::endl;
									*/
								}
							}
		}
#endif
	}
}
		}
		catch (std::runtime_error& err) {
			std::wcout << err.what() << std::endl;
		}
	} while (running);

	std::wcout << L"Driver shutdown .." << std::endl;
	ki.Exit();

	return 0;
}