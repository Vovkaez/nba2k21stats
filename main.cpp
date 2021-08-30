#include <iostream>
#include <algorithm>
#include <string>
#include <vector>
#include <Windows.h>
#include <Tlhelp32.h>

std::string stats[] = {"PTS", "REB", "AST", "STL", "BLK", "TO", "FGM", "FGMISS", "3PT", "3PTM", "3PTMISS", "FTM", "FTMISS", "OR", "FLS",
                       "+-", "PRF", "DNK", "PAINT", "LAYUP"},
        team_names[] = {"GUEST", "HOME"},
        pos_names[] = {"PG", "SG", "SF", "PF", "C"};
const CHAR *window_title = "NBA 2K21";

uint64_t stat_offset[] = {0, 0x408,  0x428,     0x420, 0x422,    1,  0x8,   0xA,  0,  0xC,    0xE,     4,     6,    0x408,     1,
                          1,     1, 0x30, 0x18, 0x32};

int N_STATS = sizeof(stat_offset) / sizeof(uint64_t);

std::vector<uint64_t> ptr_offsets = {0x30, 0x18, 0x1E0, 0};

uint64_t BASE_PLAYERS_OFFSET = 0x04D0AF88, GAME_ADDRESS;
HWND WINDOW;
HANDLE PHNDL;

void proceed_ptr(uint64_t& ptr, std::vector<uint64_t> offsets){
    for (int i = 0; i < (int)offsets.size() - 1; ++i){
        LPVOID nptr;
        ptr += offsets[i];
        ReadProcessMemory(PHNDL, (LPCVOID)ptr, &nptr, sizeof(nptr), NULL);
        ptr = (uint64_t)nptr;
    }
    ptr += offsets.back();
}

int get_stat(bool team, int pos, int stat){
    if (stat == 8)
        stat = 9;
    if (stat_offset[stat] == 1){
        std::cout << "This stat is no available\n";
        return -1;
    }
    uint64_t addr = GAME_ADDRESS;
    auto offsets = ptr_offsets;
    offsets.insert(offsets.begin(), BASE_PLAYERS_OFFSET + (int)team * 0x28 + 8 * pos);
    proceed_ptr(addr, offsets);
    addr += stat_offset[stat];
    char ans = 0;
    ReadProcessMemory(PHNDL, (LPCVOID)addr, &ans, sizeof(ans), NULL);
    return ans;
}

void set_stat(bool team, int pos, int stat, char val){
    if (stat == 8){
        set_stat(team, pos, 9, val);
        set_stat(team, pos, 10, val);
        return;
    }
    if (stat_offset[stat] == 1){
        std::cout << "This stat is no available\n";
        return;
    }
    uint64_t addr = GAME_ADDRESS;
    auto offsets = ptr_offsets;
    offsets.insert(offsets.begin(), BASE_PLAYERS_OFFSET + (int)team * 0x28 + 8 * pos);
    proceed_ptr(addr, offsets);
    addr += stat_offset[stat];
    WriteProcessMemory(PHNDL, (LPVOID)addr, &val, sizeof(val), NULL);
}

uint64_t GetModuleBaseAddress(DWORD procId, char* modName) {
    uintptr_t modBaseAddr = 0;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, procId);
    if (hSnap != INVALID_HANDLE_VALUE){
        MODULEENTRY32 modEntry;
        modEntry.dwSize = sizeof(modEntry);
        if (Module32First(hSnap, &modEntry)) {
            do {
                if (!strcmp(modEntry.szModule, modName)) {
                    modBaseAddr = (uintptr_t)modEntry.modBaseAddr;
                    break;
                }
            } while (Module32Next(hSnap, &modEntry));
        }
    }
    CloseHandle(hSnap);
    return modBaseAddr;
}
// OAKLEY - 20 AST MITCHELL
void print_stats(){
    for (int t = 0; t < 2; ++t){
        std::cout << team_names[t] << ":\n";
        for (int i = 0; i < 5; ++i){
            std::cout << pos_names[i] << '\n';
            for (int j = 0; j < N_STATS; ++j){
                if (stat_offset[j] == 1)
                    continue;
                std::cout << stats[j] << " - " << (int)get_stat(t, i, j) << '\n';
            }
        }
    }
}

void open_game(){
    WINDOW = FindWindowA(NULL, window_title);
    std::cout << std::hex << WINDOW << '\n';
    DWORD pid;
    GetWindowThreadProcessId(WINDOW, &pid);
    PHNDL = OpenProcess(PROCESS_ALL_ACCESS, false, pid);
    GAME_ADDRESS = GetModuleBaseAddress(pid, "nba2k21.exe");
}

int get_score(bool t) {
    int ans = 0;
    for (int i = 0; i < 3; ++i)
        ans += get_stat(t, i, 0);
    return ans;
}

bool confirm(std::string s){
    std::cout << s << '\n' << "[Y/N]\n";
    std::string t;
    std::cin >> s;
    return s == "Y";
}

int main(int argc, char* argv[]) {
    open_game();
    std::cout << "START\nGAME ADDRESS: " << (void*)GAME_ADDRESS << '\n';
    if (argc == 1){
        std::cout << "No arguments provided";
        return 0;
    }
    if (!strcmp(argv[1], "set") || !strcmp(argv[1], "add")){
        if (argc != 6){
            std::cout << "Wrong amount of arguments for " << argv[1] << ", need 5";
            return 0;
        }
        int t = std::find(team_names, team_names + 2, argv[2]) - team_names;
        if (t > 1){
            std::cout << "Wrong team";
            return 0;
        }
        int i = std::find(pos_names, pos_names + 5, argv[3]) - pos_names;
        if (i > 4){
            std::cout << "Wrong position";
            return 0;
        }
        int j = std::find(stats, stats + N_STATS, argv[4]) - stats;
        if (j >= N_STATS || stat_offset[j] == 1) {
            std::cout << "Wrong stat";
            return 0;
        }
        auto f = !strcmp(argv[1], "set") ? [](int x, int y) { return y; } : [](int x, int y){ return x + y; };
        int val = atoi(argv[5]);
        if (confirm("Confirm to apply stats:\n" + team_names[t] + ' ' + pos_names[i] + ' ' + stats[j] + ' ' +
                            std::to_string(f(get_stat(t, i, j), val)) + "\nPrevious: " +
                            std::to_string(get_stat(t, i, j)))){
            set_stat(t, i, j, f(get_stat(t, i, j), val));
        }
        return 0;
    }
    else if (!strcmp(argv[1], "boxscore")){
        print_stats();
    }
    else if (!strcmp(argv[1], "win")){
        if (argc != 3){
            std::cout << "Wrong amount of arguments for win need 5";
            return 0;
        }
        int t = std::find(team_names, team_names + 2, argv[2]) - team_names;
        if (t > 1){
            std::cout << "Wrong team";
            return 0;
        }
        int s[] = {get_score(0), get_score(1)};
        int ns[] = {s[0], s[1]};
        if (t) ns[1] = std::max(s[0] + 2, 21);
        else ns[0] = std::max(s[1] + 2, 21);
        if (confirm("Current score: " + std::to_string(s[0]) + " - " + std::to_string(s[1]) + "\nNew score: "
        + std::to_string(ns[0]) + " - " + std::to_string(ns[1]))) {
            set_stat(0, 0, 0, ns[0] - s[0] + get_stat(0, 0, 0));
            set_stat(1, 0, 0, ns[1] - s[1] + get_stat(1, 0, 0));
        }
    }
    else{
        std::cout << "Wrong command";
    }
    return 0;
}
