void CreateTrackingWindow();
void DestroyTrackingWindow();
void ReadWindowINI(PCSIDLWND pWindow);
void WriteWindowINI(PCSIDLWND pWindow);
PCHAR GenerateSpawnName(PSPAWNINFO pSpawn, PCHAR NameString);

void ReloadSpawn();

void Track(PSPAWNINFO pChar, PCHAR szLine);
void StopTracking(PSPAWNINFO pChar, PCHAR szLine);
void TrackSpawn();

int pulsedelay = 0;
bool IsTracking = false;
int TrackDist = 75;
int RefreshTimer = 60;

CHAR szNameTemplate[MAX_STRING] = {0};
CHAR szCustomSearch[MAX_STRING] = {0};

bool refreshWindow = false;
clock_t lastRefresh = clock();

bool Update = false;

typedef struct _TRACKSORT {
    CHAR Name[64];
    DWORD SpawnID;
} TRACKSORT, *PTRACKSORT;


PSPAWNINFO pTrackSpawn;

PCHAR szDirection[] = {
    "straight ahead",               //0
    "ahead and to the left",         //1
    "to the left",                  //2
    "behind and to the left",         //3
   "diretly behind",               //4
    "behind and to the right",         //5
    "to the right",                  //6
    "ahead and to the right"         //7
};
