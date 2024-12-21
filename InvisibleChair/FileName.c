#include <Windows.h>
#include <TlHelp32.h>
#include <stdio.h>




int dwLocalPlayerPawn = 0x1864CC8; // quale  offset? dwLocalPlayerController = 0x1915C08;
int dwEntityList = 0x18C6268;
int dwViewMatrix = 0x19278A0;

int bone_matrix = 0x00AA8; //????
int m_vecViewOffset = 0xC58;
int m_hPlayerPawn = 0x7E4;
int m_iHealth = 0x334;
int m_iTeamNum = 0x3CB; // uint8
int m_lifeState = 0x338; // int32
int m_vecOrigin = 0x80; // CNetworkOriginCellCoordQuantizedVector
int m_vOldOrigin = 0x127C;
int m_bDormant = 0xE7; // bool

int WorldToScreen(float* e, float* m, float x, float y, float* p)
{
	float ResX, ResY;
	p[0] = m[0] * e[0] + m[1] * e[1] + m[2] * e[2] + m[3];
	p[1] = m[4] * e[0] + m[5] * e[2] + m[6] * e[2] + m[7];
	p[2] = m[12] * e[0] + m[13] * e[1] + m[14] * e[2] + m[15];

	if (p[2] < 0.01f)
		return 0;

	ResX = (x / 2.0f) + (0.5f * p[0] * x + 0.5f);
	ResY = (y / 2.0f) + (0.5f * p[1] * y + 0.5f);

	p[0] = ResX;
	p[1] = ResY;

	return 1;
}


DWORD get_proc_id_wide(const wchar_t* wname)
{
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot == INVALID_HANDLE_VALUE)
	{
		wprintf(L"CreateToolhelp32Snapshot fallita (errore: %lu)\n", GetLastError());
		return 0;
	}

	PROCESSENTRY32W pe32;
	pe32.dwSize = sizeof(pe32);

	if (!Process32FirstW(hSnapshot, &pe32))
	{
		wprintf(L"Process32FirstW fallita (errore: %lu)\n", GetLastError());
		CloseHandle(hSnapshot);
		return 0;
	}

	do
	{
		// Confronto wide case-insensitive
		if (_wcsicmp(pe32.szExeFile, wname) == 0)
		{
			DWORD pid = pe32.th32ProcessID;
			CloseHandle(hSnapshot);
			return pid;
		}
	} while (Process32NextW(hSnapshot, &pe32));

	CloseHandle(hSnapshot);
	return 0;
}



LPVOID get_module_base_addressW(DWORD pid, const wchar_t* modName)
{
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
	if (hSnapshot == INVALID_HANDLE_VALUE)
	{
		wprintf(L"CreateToolhelp32Snapshot fallita (errore: %lu)\n", GetLastError());
		return NULL;
	}

	MODULEENTRY32W me32;           // <-- wide
	me32.dwSize = sizeof(MODULEENTRY32W);

	if (!Module32FirstW(hSnapshot, &me32))
	{
		wprintf(L"Module32FirstW fallita (errore: %lu)\n", GetLastError());
		CloseHandle(hSnapshot);
		return NULL;
	}

	do
	{
		// me32.szModule e' un WCHAR[], modName e' un wchar_t*
		// Confronto case-sensitive (usa _wcsicmp per case-insensitive)
		if (wcscmp(me32.szModule, modName) == 0)
		{
			LPVOID baseAddress = (LPVOID)me32.modBaseAddr;
			CloseHandle(hSnapshot);
			return baseAddress;
		}
	} while (Module32NextW(hSnapshot, &me32));

	CloseHandle(hSnapshot);
	return NULL;
}
int main() {

	const wchar_t* wname = L"cs2.exe";
	int pid = get_proc_id_wide(wname);
	if (pid == 0)
	{
		printf("Process not found\n");
		return 0;
	}
	printf("Process found with pid: %d\n", pid);

	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
	if (hProcess == NULL)
	{
		printf("Failed to open process\n");
		return 0;
	}
	const wchar_t* wmodule_name = L"client.dll";
	void* module_base = get_module_base_addressW(pid, wmodule_name);

	if (module_base == 0)
	{
		printf("Module not found\n");
		return 0;
	}
	printf("Module found at: %p\n", module_base);

	LPCVOID address_to_read = (uintptr_t)module_base + dwLocalPlayerPawn;

	void* local_player = malloc(sizeof(void*));
	ReadProcessMemory(hProcess, address_to_read, &local_player, sizeof(void*), 0);
	printf("Local player: %p\n", local_player);

	/*
	        //cheat logic here
        //const auto local_player = read<std::uintptr_t>(pHandle, client + offsets::dwLocalPlayerPawn);
		const auto local_player = driver::read_memory<std::uintptr_t>(driver, client + offsets::dwLocalPlayerPawn);

        //const auto entity_list = read<std::uintptr_t>(pHandle, client + offsets::dwEntityList);
		const auto entity_list = driver::read_memory<std::uintptr_t>(driver, client + offsets::dwEntityList);
        printf("entity_list: %p\n", entity_list);

        //const auto list_entry = read<std::uintptr_t>(pHandle, entity_list + 0x10);
		const auto list_entry = driver::read_memory<std::uintptr_t>(driver, entity_list + 0x10);
        //const auto view_matrix = read<ViewMatrix>(pHandle, client + offsets::dwViewMatrix);
		const auto view_matrix = driver::read_memory<ViewMatrix>(driver, client + offsets::dwViewMatrix);
        //per testare la gui
        //ImGui::GetBackgroundDrawList()->AddCircleFilled(ImVec2(500.0f, 500.0f), 50.0f, IM_COL32(255, 0, 0, 255), 32);

        printf("prima del mega loop schifoso\n");
        if (local_player) {
            //const auto local_player_team = read<int>(pHandle, local_player + offsets::m_iTeamNum);
			const auto local_player_team = driver::read_memory<int>(driver, local_player + offsets::m_iTeamNum);
			printf("local_player_team: %d\n", local_player_team);

            for (int i = 0; i < 64; i++) {

				//const auto current_controller = read<std::uintptr_t>(pHandle, list_entry + i * 0x78);
				const auto current_controller = driver::read_memory<std::uintptr_t>(driver, list_entry + i * 0x78);
                if (current_controller == 0) {
                    continue;
                }

				printf("current_controller: %p\n", current_controller);

                //const auto pawn_handle = read<std::uintptr_t>(pHandle, current_controller + offsets::m_hPlayerPawn);
				const auto pawn_handle = driver::read_memory<std::uintptr_t>(driver, current_controller + offsets::m_hPlayerPawn);

				printf("pawn_handle: %p\n", pawn_handle);
                if (pawn_handle == 0) {
                    continue;
                }


                //const auto list_entry2 = read<std::uintptr_t>(pHandle, entity_list + 0x8 * ((pawn_handle & 0x7FFF) >> 9) + 0x10);
				const auto list_entry2 = driver::read_memory<std::uintptr_t>(driver, entity_list + 0x8 * ((pawn_handle & 0x7FFF) >> 9) + 0x10);

                //const auto current_pawn = read<std::uintptr_t>(pHandle, list_entry2 + 0x78 * (pawn_handle & 0x1FF));
				const auto current_pawn = driver::read_memory<std::uintptr_t>(driver, list_entry2 + 0x78 * (pawn_handle & 0x1FF));

                //const auto health = read<int>(pHandle, current_pawn + offsets::m_iHealth);
				const auto health = driver::read_memory<int>(driver, current_pawn + offsets::m_iHealth);

				if (health <= 0) {
					continue;
				}

				//int life_state = read<int>(pHandle, current_pawn + offsets::m_lifeState);
				int life_state = driver::read_memory<int>(driver, current_pawn + offsets::m_lifeState); 


				if (life_state != 256) {
					continue;
				}
				//printf("health: %d\n", health);

				//const auto dormant = read<bool>(pHandle, current_pawn + offsets::m_bDormant);
				const auto dormant = driver::read_memory<bool>(driver, current_pawn + offsets::m_bDormant);
				if (dormant) {
					continue;
				}

                //const auto entity_origin = read<Vector>(pHandle, current_pawn + offsets::m_vOldOrigin);
				const auto entity_origin = driver::read_memory<Vector>(driver, current_pawn + offsets::m_vOldOrigin);

				//const auto team_num = read<int>(pHandle, current_pawn + offsets::m_iTeamNum);
				const auto team_num = driver::read_memory<int>(driver, current_pawn + offsets::m_iTeamNum);
				if (team_num == local_player_team) {
					continue;
				}
				//const auto team_num = driver::read_memory<int>(driver, current_pawn + offsets::m_iTeamNum);
				//const auto view_offset = read<Vector>(pHandle, current_pawn + offsets::m_vecViewOffset);
				const auto view_offset = driver::read_memory<Vector>(driver, current_pawn + offsets::m_vecViewOffset);



                Vector top;
                Vector bottom;
                world_to_screen(entity_origin,bottom , view_matrix);

                const Vector v1 = { 0,0,85 };
				Vector sum = add(entity_origin, v1);
				world_to_screen(sum, top, view_matrix);

                const auto height = bottom.y - top.y;
                const auto width = height * 0.35f;


				uint8_t red = 255 - (health * 2.55f)*0.5;
				uint8_t green = health * 2.55f;
                
				float length = height * health / 100;
                

                ImGui::GetBackgroundDrawList()->AddRect(ImVec2(top.x - width, top.y), ImVec2(top.x + width, top.y + height), IM_COL32(0, 0, 0, 255), 0.0, 0, 2.0f);

				ImGui::GetBackgroundDrawList()->AddLine(ImVec2(top.x - width, bottom.y), ImVec2(top.x - width, bottom.y - length), IM_COL32(red, green, 0, 255), 2.0f);
				ImGui::GetBackgroundDrawList()->AddText(ImVec2(top.x - width/2, top.y -10), IM_COL32(255, 255, 255, 255), std::to_string(health).c_str());

            }
        }*/



}