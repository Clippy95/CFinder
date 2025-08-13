// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include <array>
#include "MemoryMgr.h"
#include "GameConfig.h"
#include "BlingMenu_public.h"
#define CFinder_Version "r4"
struct vector2 {
	float x;
	float y;
};

struct vector3
{
	float x;
	float y;
	float z;
};
typedef float(__cdecl* ChangeTextColorT)(int R, int G, int B, int Alpha);
ChangeTextColorT ChangeTextColor = (ChangeTextColorT)0xD14840;

struct matrix
{
	vector3 rvec;  // Right vector
	vector3 uvec;  // Up vector
	vector3 fvec;  // Forward vector
};

struct TagStateFlags
{
	unsigned __int8 disabled : 1;
	unsigned __int8 tagged : 1;
};

struct TagPlacementFlags
{
	unsigned __int8 ambient : 1;
	unsigned __int8 disabled : 1;
};


struct TagPlacement
{
	int uid;
	vector3 position;
	matrix orientation;
	int team;
	TagPlacementFlags flags;
	unsigned __int16 chunk_uid;
};


struct TagState
{
	TagStateFlags flags;
	TagPlacement* script_object;
	unsigned int trigger_handle;
	unsigned int tagged_function_handle;
};

vector2* __cdecl ProjectToPauseMap(vector2* location, float x, float y) {
	return ((vector2 * (__cdecl*)(vector2*, float, float))0x771490)(location, x, y);
}

char __cdecl gr_rect(int x1, int y1, int w, int h) {
	return ((char(__cdecl*)(int, int, int, int, int))0xD0B980)(x1, y1, w, h, 0xEC2740);
}

int __cdecl gr_circle(int x1, int y1, float eh = 0) {
	return ((int(__cdecl*)(int, int))0xD11090)(x1, y1);
}

// returns amount of spots there are, max is 50 only
int __cdecl GetTagStates(TagState* array) {
	return ((char(__fastcall*)(int, TagState*))0x626C80)(0, array);
}



enum CompletionState : __int32
{
	DCP_NONE = 0x0,
	DCP_ATTEMPTED = 0x1,
	DCP_COMPLETED = 0x2,
	DCP_RACING_BRONZE = 0x3,
	DCP_RACING_SILVER = 0x4,
	DCP_RACING_GOLD = 0x5,
	DCP_NUM_TYPES = 0x6,
};

struct CollectibleProgress
{
	vector3 world_pos;
	CompletionState progress;
};

struct CDProgressEntry
{
	int32_t cd_chksum;
	CollectibleProgress completion_info;
};

CDProgressEntry* CDProgressEntries = (CDProgressEntry*)0x27A0C90;

int __cdecl GetCDProgress(CollectibleProgress* completion_array, int max_size)
{
	CollectibleProgress* p_completion_info;
	CollectibleProgress* v3;
	int i;
	int cds_count;

	cds_count = 0;
	for (i = 0;
		i < 60
		&& i < max_size
		&& !(CDProgressEntries[i].cd_chksum == -1);
		++i)
	{
		p_completion_info = &CDProgressEntries[i].completion_info;
		v3 = &completion_array[i];
		v3->world_pos.x = p_completion_info->world_pos.x;
		v3->world_pos.y = p_completion_info->world_pos.y;
		v3->world_pos.z = p_completion_info->world_pos.z;
		v3->progress = p_completion_info->progress;
		++cds_count;
	}
	return cds_count;
}

typedef int(__cdecl* CompletionsT)(CollectibleProgress* array, int max_size);



bool __declspec(naked) ProjectToMinimapRaw(vector3* world_pos, float* fade_percent, vector2* mini_pos, float* unclipped_pos) {
	static int worldpos_addr = 0x79EC50;
	__asm {
		push ebp
		mov ebp, esp
		sub esp, __LOCAL_SIZE


		mov eax, world_pos
		mov ecx, fade_percent
		push unclipped_pos
		push mini_pos

		call worldpos_addr


		mov esp, ebp
		pop ebp
		ret
	}
}

bool ProjectToMinimap(vector3* world_pos, float* fade_percent, vector2* mini_pos, float* unclipped_pos) {
	return !ProjectToMinimapRaw(world_pos, fade_percent, mini_pos, unclipped_pos);
}


typedef char(__thiscall* ImageLookupFn)(DWORD* image_table, DWORD* image_entry, unsigned int* hash, int lookup_flags);
ImageLookupFn FindImageEntry = (ImageLookupFn)0xB89F80;

typedef char __cdecl DrawMapImageFn(unsigned int image_id, float cx, float cy, float angle, float scale, int render_state);
DrawMapImageFn* DrawMapImage = (DrawMapImageFn*)0xB87C10;


int collectible_image_ids[4] = { -1, -1, -1, -1 };
const char* collectible_image_names[4] = {
	"ui_cFinder_map_cd",
	"ui_cFinder_map_tagging",
	"ui_cFinder_map_barn",
	"ui_cFinder_map_stunt"
};


enum CollectibleType {
	COLLECTIBLE_CD = 0,
	COLLECTIBLE_TAGGING = 1,
	COLLECTIBLE_BARN = 2,
	COLLECTIBLE_STUNT = 3,
	COLLECTIBLE_COUNT
};


void __declspec(naked) HashImageName(unsigned int seed, char* text, unsigned int* hash_out) {
	static int string_addr = 0x00BDCB30;
	__asm {
		push ebp
		mov ebp, esp

		mov eax, seed
		mov ecx, text
		push hash_out

		call string_addr

		mov esp, ebp
		pop ebp
		ret
	}
}

unsigned int FindMapImage(const char* texname) {
	unsigned int hashout;
	HashImageName(0, (char*)texname, &hashout);
	DWORD image_entry = 0;
	if (FindImageEntry((DWORD*)0xEBDD3C, &image_entry, &hashout, 0) && image_entry)
		return *(DWORD*)image_entry;
	else return -1;
}

struct CollectibleSettings {
    union {
        struct {
            uint32_t show_tags : 1;
            uint32_t show_cds : 1;
            uint32_t show_barns : 1;
            uint32_t show_stunts : 1;
            uint32_t show_in_minimap : 1;
            uint32_t show_in_pausemap : 1;
            uint32_t hide_completed : 1;
            uint32_t show_only_completed : 1; // NEW: Only show completed items
            uint32_t bling_loaded : 1;
        };
        uint32_t flags;
    };
    int completed_alpha;
    float pausemap_scale;
    float minimap_scale;
} g_CollectibleConfig;

// Update the config initialization
void InitCollectibleConfig() {
    g_CollectibleConfig.show_tags = GameConfig::GetValue("Collectibles", "ShowTags", 1);
    g_CollectibleConfig.show_cds = GameConfig::GetValue("Collectibles", "ShowCDs", 1);
    g_CollectibleConfig.show_barns = GameConfig::GetValue("Collectibles", "ShowBarns", 1);
    g_CollectibleConfig.show_stunts = GameConfig::GetValue("Collectibles", "ShowStunts", 1);
    g_CollectibleConfig.show_in_minimap = GameConfig::GetValue("Collectibles", "ShowInMinimap", 1);
    g_CollectibleConfig.show_in_pausemap = GameConfig::GetValue("Collectibles", "ShowInPauseMap", 1);
    g_CollectibleConfig.hide_completed = GameConfig::GetValue("Collectibles", "HideCompleted", 0);
    g_CollectibleConfig.show_only_completed = GameConfig::GetValue("Collectibles", "ShowOnlyCompleted", 0); // NEW
    g_CollectibleConfig.completed_alpha = std::clamp((int)GameConfig::GetValue("Collectibles", "CompletedAlpha", 64), 0, 255);
    g_CollectibleConfig.pausemap_scale = (float)GameConfig::GetDoubleValue("Collectibles", "PauseMapScale", 0.6);
    g_CollectibleConfig.minimap_scale = (float)GameConfig::GetDoubleValue("Collectibles", "MinimapScale", 0.3);
}

#define MAX_MAP_ZOOM_IN *(float*)0x1F7A940
// Updated render functions
CollectibleProgress completions[80];
void RenderCollectibles_Stunt_And_Barn(bool is_minimap = false) {
    CompletionsT* func_array = (CompletionsT*)0xE8DDEC;

    // Check if we should render barns
    if (g_CollectibleConfig.show_barns) {
        int result0 = func_array[0](completions, 100);
        vector2 map_loc{};
        float fade_percent = 1.0f;

        for (int i = 0; i < result0; i++) {
            bool is_completed = (completions[i].progress == DCP_COMPLETED);

            if (is_completed && g_CollectibleConfig.hide_completed) continue;

            if (g_CollectibleConfig.show_only_completed && !is_completed) continue;
            if (is_completed && g_CollectibleConfig.hide_completed) continue;

            if (is_minimap) {
                if (!ProjectToMinimap(&completions[i].world_pos, &fade_percent, &map_loc, NULL)) {
                    continue;
                }
            }
            else {
                ProjectToPauseMap(&map_loc, completions[i].world_pos.x, completions[i].world_pos.z);
            }

            int alpha = is_completed ? g_CollectibleConfig.completed_alpha : 255;

            if (collectible_image_ids[COLLECTIBLE_BARN] >= 0) {
                                float base_scale = is_minimap ? g_CollectibleConfig.minimap_scale : g_CollectibleConfig.pausemap_scale;
                float scale = base_scale;

                if (!is_minimap) {
                    float map_zoom = *(float*)0xE8DFA8;
                    scale = base_scale * (map_zoom / MAX_MAP_ZOOM_IN);
                }

                ChangeTextColor(255, 255, 255, alpha);
                DrawMapImage(
                    collectible_image_ids[COLLECTIBLE_BARN],
                    map_loc.x, map_loc.y, 0.0f, scale, 0xEC2740
                );
            }
            else {
                                float base_scale = is_minimap ? g_CollectibleConfig.minimap_scale : g_CollectibleConfig.pausemap_scale;
                float scale = base_scale;

                if (!is_minimap) {
                    float map_zoom = *(float*)0xE8DFA8;
                    scale = base_scale * (map_zoom / MAX_MAP_ZOOM_IN);
                }

                ChangeTextColor(255, 255, 255, alpha);
                DrawMapImage(-1, map_loc.x, map_loc.y, 0.0f, scale, 0xEC2740);
            }
        }
    }

    // Check if we should render stunts
    if (g_CollectibleConfig.show_stunts) {
        int result3 = func_array[3](completions, 100);

        for (int i = 0; i < result3; i++) {
            bool is_completed = (completions[i].progress == DCP_COMPLETED);

            if (is_completed && g_CollectibleConfig.hide_completed) continue;

            if (g_CollectibleConfig.show_only_completed && !is_completed) continue;
            if (is_completed && g_CollectibleConfig.hide_completed) continue;

            vector2 map_loc{};
            float fade_percent = 1.0f;

            if (is_minimap) {
                if (!ProjectToMinimap(&completions[i].world_pos, &fade_percent, &map_loc, NULL)) {
                    continue;
                }
            }
            else {
                ProjectToPauseMap(&map_loc, completions[i].world_pos.x, completions[i].world_pos.z);
            }

            int alpha = is_completed ? g_CollectibleConfig.completed_alpha : 255;

            if (collectible_image_ids[COLLECTIBLE_STUNT] >= 0) {
                                float base_scale = is_minimap ? g_CollectibleConfig.minimap_scale : g_CollectibleConfig.pausemap_scale;
                float scale = base_scale;

                if (!is_minimap) {
                    float map_zoom = *(float*)0xE8DFA8;
                    scale = base_scale * (map_zoom / MAX_MAP_ZOOM_IN);
                }

                ChangeTextColor(255, 255, 255, alpha);
                DrawMapImage(
                    collectible_image_ids[COLLECTIBLE_STUNT],
                    map_loc.x, map_loc.y, 0.0f, scale, 0xEC2740
                );
            }
            else {
              float base_scale = is_minimap ? g_CollectibleConfig.minimap_scale : g_CollectibleConfig.pausemap_scale;
                float scale = base_scale;

                if (!is_minimap) {
                    float map_zoom = *(float*)0xE8DFA8;
                    scale = base_scale * (map_zoom / MAX_MAP_ZOOM_IN);
                }

                ChangeTextColor(0, 0, 255, alpha);
                DrawMapImage(-1, map_loc.x, map_loc.y, 0.0f, scale, 0xEC2740);
            }
        }
    }
}

void RenderCollectiblesCDs(bool is_minimap = false) {
    if (!g_CollectibleConfig.show_cds) return;

    CompletionsT* func_array = (CompletionsT*)0xE8DDEC;
    CollectibleProgress completions[50];

    // Call function [1] for CDs
    int result1 = func_array[1](completions, 50);

    vector2 map_loc{};
    float fade_percent = 1.0f;

    for (int i = 0; i < result1; i++) {
        bool is_completed = (completions[i].progress == DCP_COMPLETED);

        if (g_CollectibleConfig.show_only_completed && !is_completed) continue;
        if (is_completed && g_CollectibleConfig.hide_completed) continue;

        if (is_minimap) {
            if (!ProjectToMinimap(&completions[i].world_pos, &fade_percent, &map_loc, NULL)) {
                continue;
            }
        }
        else {
            ProjectToPauseMap(&map_loc, completions[i].world_pos.x, completions[i].world_pos.z);
        }

        int alpha = is_completed ? g_CollectibleConfig.completed_alpha : 255;

        if (collectible_image_ids[COLLECTIBLE_CD] >= 0) {
                            float base_scale = is_minimap ? g_CollectibleConfig.minimap_scale : g_CollectibleConfig.pausemap_scale;
            float scale = base_scale;

            if (!is_minimap) {
                float map_zoom = *(float*)0xE8DFA8;
                scale = base_scale * (map_zoom / MAX_MAP_ZOOM_IN);
            }

            ChangeTextColor(255, 255, 255, alpha);
            DrawMapImage(
                collectible_image_ids[COLLECTIBLE_CD],
                map_loc.x, map_loc.y, 0.0f, scale, 0xEC2740
            );
        }
        else {
                            float base_scale = is_minimap ? g_CollectibleConfig.minimap_scale : g_CollectibleConfig.pausemap_scale;
            float scale = base_scale;

            if (!is_minimap) {
                float map_zoom = *(float*)0xE8DFA8;
                scale = base_scale * (map_zoom / MAX_MAP_ZOOM_IN);
            }

            ChangeTextColor(255, 0, 0, alpha);
            DrawMapImage(-1, map_loc.x, map_loc.y, 0.0f, scale, 0xEC2740);
        }
    }
}

void RenderCollectiblesTags(bool is_minimap = false) {
    if (!g_CollectibleConfig.show_tags) return;

    TagState spots_to_track[50]{};
    int amount_of_tags = GetTagStates(spots_to_track);

    vector2 map_loc{};
    float fade_percent = 1.0f;

    for (int i = 0; i < amount_of_tags; i++) {
        TagPlacement* this_spot = spots_to_track[i].script_object;
        if (this_spot == NULL) {
            continue;
        }

        bool is_completed = spots_to_track[i].flags.tagged;

        if (is_completed && g_CollectibleConfig.hide_completed) continue;

        if (g_CollectibleConfig.show_only_completed && !is_completed) continue;
        if (is_completed && g_CollectibleConfig.hide_completed) continue;

        vector3 pos = { this_spot->position.x, this_spot->position.y, this_spot->position.z };

        if (is_minimap) {
            if (!ProjectToMinimap(&pos, &fade_percent, &map_loc, NULL)) {
                continue;
            }
        }
        else {
            ProjectToPauseMap(&map_loc, this_spot->position.x, this_spot->position.z);
        }

        int alpha = is_completed ? g_CollectibleConfig.completed_alpha : 255;

        if (collectible_image_ids[COLLECTIBLE_TAGGING] >= 0) {
                            float base_scale = is_minimap ? g_CollectibleConfig.minimap_scale : g_CollectibleConfig.pausemap_scale;
            float scale = base_scale;

            if (!is_minimap) {
                float map_zoom = *(float*)0xE8DFA8;
                scale = base_scale * (map_zoom / MAX_MAP_ZOOM_IN);
            }

            ChangeTextColor(255, 255, 255, alpha);
            DrawMapImage(
                collectible_image_ids[COLLECTIBLE_TAGGING],
                map_loc.x, map_loc.y, 0.0f, scale, 0xEC2740
            );
        }
        else {
                            float base_scale = is_minimap ? g_CollectibleConfig.minimap_scale : g_CollectibleConfig.pausemap_scale;
            float scale = base_scale;

            if (!is_minimap) {
                float map_zoom = *(float*)0xE8DFA8;
                scale = base_scale * (map_zoom / MAX_MAP_ZOOM_IN);
            }

            if (spots_to_track[i].flags.tagged) {
                ChangeTextColor(106, 90, 205, alpha); // Purple for tagged
            }
            else {
                ChangeTextColor(255, 0, 0, alpha); // Red for untagged
            }

            DrawMapImage(-1, map_loc.x, map_loc.y, 0.0f, scale, 0xEC2740);
        }
    }
}

void RenderCollectibles(bool is_minimap = false) {
    // Check if we should render in this context
    if (is_minimap && !g_CollectibleConfig.show_in_minimap) return;
    if (!is_minimap && !g_CollectibleConfig.show_in_pausemap) return;

    RenderCollectiblesTags(is_minimap);
    RenderCollectiblesCDs(is_minimap);
    RenderCollectibles_Stunt_And_Barn(is_minimap);
}

typedef int(*CollectibleInitFn)();
CollectibleInitFn OriginalCollectibleInit;

typedef uintptr_t(*InterfaceInitFn)();
InterfaceInitFn OriginalInterfaceInit;



typedef bool(__fastcall* TextureLoadFn)(const char* filename, uintptr_t mempool);
TextureLoadFn OriginalTextureLoad;

__declspec(naked) void LoadBitmapTable(const char* FileName) {


	__asm {

		push ebp
		mov ebp, esp

		sub esp, __LOCAL_SIZE
		mov eax, FileName

		mov edx, 0xB87540

		call edx

		mov esp, ebp

		pop ebp

		ret

	}
}

void LoadExtraBitMapTable(const char* fileName) {
    // Preserve all five bytes, including any DLC/mod hook already installed here.
    std::array<uint8_t, 5> saved;
    memcpy(saved.data(), reinterpret_cast<const void*>(0xB875B0), saved.size());
    Memory::VP::InjectHook(0xB875B0, 0xB875C4, Memory::VP::HookType::Jump);
    FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<void*>(0xB875B0), saved.size());
    LoadBitmapTable(fileName);
    Memory::VP::Patch(0xB875B0, saved);
    FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<void*>(0xB875B0), saved.size());
}



uintptr_t LoadCollectibleImageTable() {

	LoadExtraBitMapTable("cfinder-ui.xtbl");
	return OriginalInterfaceInit();
}

bool __fastcall LoadCollectibleTextures(const char* filename, uintptr_t mempool) {

	if (mempool == 0x27716E4 && strcmp(filename, "interface-backend.peg") == 0) {
		OriginalTextureLoad(filename, mempool);
		return OriginalTextureLoad("cfinder-ui.peg", mempool);
	}

	return OriginalTextureLoad(filename, mempool);
}

int InitCollectibleImages() {
    for (int i = 0; i < COLLECTIBLE_COUNT; i++) {
        collectible_image_ids[i] = FindMapImage(collectible_image_names[i]);
        printf("Loaded %s with ID: %d\n", collectible_image_names[i], collectible_image_ids[i]);
    }

    InitCollectibleConfig();

    if (!g_CollectibleConfig.bling_loaded) {
        if (BlingMenuLoad()) {
            g_CollectibleConfig.bling_loaded = 1;
            BlingMenuAddFunc("CFinder", "Reload Settings", InitCollectibleConfig);
            BlingMenuAddFunc("CFinder", CFinder_Version, NULL);
        }
    }

    return OriginalCollectibleInit();
}

void __cdecl DrawMinimapCollectibles(int context, float scale, DWORD* render_data) {
	RenderCollectibles(true);
    return ((void(__cdecl*)(int,float,DWORD*))0x79F890)(context, scale, render_data);
}

void __cdecl DrawPauseMapCollectibles(int context, char pass, char options) {
	RenderCollectibles(false);
    return ((void(__cdecl*)(int, char, char))0x7A3D80)(context, pass, options);
}

void CFinder_HookStart() {
	GameConfig::Initialize();
    if (MH_Initialize() != MH_OK) {
        MessageBoxW(NULL, L"FAILED TO INITIALIZE", L"Error", MB_OK | MB_ICONERROR);
        return;
    }
    if (MH_CreateHook((LPVOID)0x696C70, &InitCollectibleImages, (LPVOID*)&OriginalCollectibleInit) != MH_OK) {
        MessageBoxW(NULL, L"FAILED TO HOOK", L"Error", MB_OK | MB_ICONERROR);
        return;
    }

	if (MH_CreateHook((LPVOID)0x522450, &LoadCollectibleTextures, (LPVOID*)&OriginalTextureLoad) != MH_OK) {
		MessageBoxW(NULL, L"FAILED TO HOOK", L"Error", MB_OK | MB_ICONERROR);
		return;
	}

    // Use the call site so Juiced Patch can keep its font-init entry hook.
    Memory::VP::InterceptCall(0x51F651, OriginalInterfaceInit, LoadCollectibleImageTable);

    if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK) {
        MessageBoxW(NULL, L"FAILED TO ENABLE", L"Error", MB_OK | MB_ICONERROR);
        return;
    }
    Memory::VP::InjectHook(0x7A3A4F, &DrawMinimapCollectibles, Memory::VP::HookType::Call);
    Memory::VP::InjectHook(0x77021B, &DrawPauseMapCollectibles, Memory::VP::HookType::Call);
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH: {
        CFinder_HookStart();
        break;
    }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

