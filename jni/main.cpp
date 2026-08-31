#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <android/log.h>

#define TAG "ESP"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)

#define OFF_PLAYER_MANAGER 180740496ULL
#define OFF_PM_LOCAL_PLAYER 0x70
#define OFF_PM_PLAYER_LIST 0x28
#define OFF_LIST_COUNT 0x20
#define OFF_LIST_BUFFER 0x18
#define OFF_LIST_ENTRY_BASE 0x30
#define OFF_LIST_ENTRY_STRIDE 0x18
#define OFF_PLAYER_TEAM 0x79
#define OFF_PLAYER_HEALTH 0x7C
#define OFF_PLAYER_MOVEMENT_CTRL 0x98
#define OFF_MC_TRANSFORM_DATA 0xB0
#define OFF_TD_POSITION 0x44

int pid = -1;
uint64_t lib = 0;

bool mem_read(uint64_t a, void* b, size_t l) {
    if(pid < 0 || !a || !b || !l || a < 0x1000) return false;
    iovec loc[1], rem[1];
    loc[0].iov_base = b; loc[0].iov_len = l;
    rem[0].iov_base = (void*)a; rem[0].iov_len = l;
    return process_vm_readv(pid, loc, 1, rem, 1, 0) == (long)l;
}

template<typename T> T rpm(uint64_t a) { T v{}; mem_read(a, &v, sizeof(T)); return v; }

int get_pid() {
    FILE* fp = popen("pidof com.axlebolt.standoff2", "r");
    if(!fp) return -1;
    int p = -1;
    fscanf(fp, "%d", &p);
    pclose(fp);
    return p;
}

uint64_t get_lib() {
    char path[64];
    snprintf(path, 64, "/proc/%d/maps", pid);
    FILE* fp = fopen(path, "r");
    if(!fp) return 0;
    char line[512];
    uint64_t base = 0;
    while(fgets(line, 512, fp)) {
        if(strstr(line, "libunity.so")) {
            sscanf(line, "%llx", (unsigned long long*)&base);
            break;
        }
    }
    fclose(fp);
    return base;
}

uint64_t get_player_manager() {
    if(!lib) return 0;
    uint64_t cls = rpm<uint64_t>(lib + OFF_PLAYER_MANAGER);
    if(!cls) return 0;
    uint64_t obj = rpm<uint64_t>(cls + 0x90);
    if(!obj) return 0;
    uint64_t fld = rpm<uint64_t>(obj + 0x10);
    return fld;
}

int main(int argc, char* argv[]) {
    if(argc > 1) pid = atoi(argv[1]);
    LOGI("ESP starting...");
    while(true) {
        if(pid < 0 || lib == 0) {
            pid = get_pid();
            if(pid > 0) lib = get_lib();
            LOGI("Waiting for game... pid=%d lib=%llu", pid, (unsigned long long)lib);
            sleep(3);
            continue;
        }
        
        uint64_t pm = get_player_manager();
        if(!pm) {
            LOGI("Player manager not found");
            sleep(3);
            continue;
        }
        
        uint64_t list = rpm<uint64_t>(pm + OFF_PM_PLAYER_LIST);
        int count = rpm<int>(pm + OFF_LIST_COUNT);
        uint64_t buffer = rpm<uint64_t>(list + OFF_LIST_BUFFER);
        
        LOGI("Players: %d", count);
        
        for(int i = 0; i < count && i < 20; i++) {
            uint64_t entry = buffer + i * OFF_LIST_ENTRY_STRIDE;
            uint64_t player = rpm<uint64_t>(entry + OFF_LIST_ENTRY_BASE);
            if(!player) continue;
            
            int team = rpm<uint8_t>(player + OFF_PLAYER_TEAM);
            float health = rpm<float>(player + OFF_PLAYER_HEALTH);
            
            uint64_t mc = rpm<uint64_t>(player + OFF_PLAYER_MOVEMENT_CTRL);
            uint64_t td = rpm<uint64_t>(mc + OFF_MC_TRANSFORM_DATA);
            float x = rpm<float>(td + OFF_TD_POSITION);
            float y = rpm<float>(td + OFF_TD_POSITION + 4);
            float z = rpm<float>(td + OFF_TD_POSITION + 8);
            
            LOGI("Player %d: team=%d hp=%.0f pos=%.1f %.1f %.1f", i, team, health, x, y, z);
        }
        
        usleep(100000);
    }
    return 0;
}
