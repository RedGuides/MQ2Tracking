// MQ2Tracking.cpp : Defines the entry point for the DLL application.
// v0.1  - Initial Release
// v0.2  - Minor bug fixes
// v0.3  - Updated to use new lists for improved performance.
// v0.4  - Added a few more sort options and a couple more filters
// v0.5  - Added customizable timed refresh based on Bushdaka's suggestion and code
// v0.6  - Changed the Cancel button to offer to a refresh function and change its label based on tracking state
// v0.7  - Added customizable entry display to the INI file.
// v0.8  - Added command /tracknames to set the display information while in game.
// v0.9  - Added custom track filter a la MP2Map style
// v0.10 - Cleaned up the commands and moved most inside the /track command.  Also added /track help
// v0.11 - Fixed Function errors caused by naming functions the same as MQ2 functions.  Bad MacroFiend!
// v0.12 - Updated for new Gray/Grey TSS options for con... bushdaka
// v0.13 - Fixed con white tracking option.
// v0.14 - Fixed window display out of sync.
// v0.15 - Added in code for autoupdate, adjustments for saving options.
// v0.16 - posix/securecrt.

#include <time.h>
#include "../MQ2Plugin.h"
PreSetup("MQ2Tracking");
PLUGIN_VERSION(0.16);

void CreateTrackingWindow();
void DestroyTrackingWindow();
void ReadWindowINI(PCSIDLWND pWindow);
void WriteWindowINI(PCSIDLWND pWindow);
PCHAR GenerateSpawnName(PSPAWNINFO pSpawn, PCHAR NameString);
void ReloadSpawn();
void Track(PSPAWNINFO pChar, PCHAR szLine);
void StopTracking(PSPAWNINFO pChar, PCHAR szLine);
void TrackSpawn();

bool IsTracking = false;
int pulsedelay = 0;
int TrackDist = 75;
int RefreshTimer = 60;
CHAR szNameTemplate[MAX_STRING] = { 0 };
CHAR szCustomSearch[MAX_STRING] = { 0 };
clock_t lastRefresh = clock();
PSPAWNINFO pTrackSpawn;

PCHAR szDirection[] = {
   "straight ahead",         //0
   "ahead and to the left",   //1
   "to the left",            //2
   "behind and to the left",   //3
   "diretly behind",         //4
   "behind and to the right",   //5
   "to the right",            //6
   "ahead and to the right"   //7
};

typedef struct _TRACKSORT {
   CHAR Name[64];
   DWORD SpawnID;
} TRACKSORT, *PTRACKSORT;

PTRACKSORT pTrackSort;

// Window Declarations
class CMyTrackingWnd : public CCustomWnd
{
public:
   CMyTrackingWnd():CCustomWnd("TrackingWnd")
   {
      FilterRedButton=(CButtonWnd*)GetChildItem("TRW_FilterRedButton");
      FilterYellowButton=(CButtonWnd*)GetChildItem("TRW_FilterYellowButton");
      FilterWhiteButton=(CButtonWnd*)GetChildItem("TRW_FilterWhiteButton");
      FilterBlueButton=(CButtonWnd*)GetChildItem("TRW_FilterBlueButton");
      FilterLBlueButton=(CButtonWnd*)GetChildItem("TRW_FilterLightBlueButton");
      FilterGreenButton=(CButtonWnd*)GetChildItem("TRW_FilterGreenButton");
      FilterGrayButton=(CButtonWnd*)GetChildItem("TRW_FilterGray");
      TrackingList=(CListWnd*)GetChildItem("TRW_TrackingList");
      TrackSortCombo=(CComboWnd*)GetChildItem("TRW_TrackSortCombobox");
      TrackPlayersCombo=(CComboWnd*)GetChildItem("TRW_TrackPlayersCombobox");
      TrackButton=(CButtonWnd*)GetChildItem("TRW_TrackButton");
      DoneButton=(CButtonWnd*)GetChildItem("DoneButton");
      AutoUpdateButton = (CButtonWnd*)GetChildItem("TRW_AutoUpdateButton");
      SetWndNotification(CMyTrackingWnd);
   }

   ~CMyTrackingWnd()
   {
   }

   int WndNotification(CXWnd *pWnd, unsigned int Message, void *unknown) {
      if (pWnd==0) {
         if (Message==XWM_CLOSE) {
            CreateTrackingWindow();
            ((CXWnd*)this)->Show(1,1);
            SetVisible(true);
            return 1;
         }
      }
      if (pWnd == (CXWnd*)TrackButton) {
         if (Message==XWM_LCLICK) {
            CHAR szLine[MAX_STRING] = {0};
            pTrackSpawn = (PSPAWNINFO)GetSpawnByID(pTrackSort[TrackingList->GetCurSel()].SpawnID);
            if (pTrackSpawn) {
               WriteChatColor("Tracking started.");
               TrackSpawn();
            }
         }
      }
      else if (pWnd == (CXWnd*)DoneButton) {
         if (Message==XWM_LCLICK) {
            StopTracking(NULL,NULL);
            ReloadSpawn();
         }
      }
      else if (pWnd == (CXWnd*)AutoUpdateButton) {
         if (Message == XWM_LCLICK) {
            // toggle automatic update
         }
      }
      else if (pWnd == (CXWnd*)TrackingList) {
         if (Message==XWM_RCLICK) {
            char szMsg[MAX_STRING] = { 0 };
            PSPAWNINFO pSpawnTrack = (PSPAWNINFO)GetSpawnByID(pTrackSort[TrackingList->GetCurSel()].SpawnID);
            if (pSpawnTrack) {
               if (IsTargetable(pSpawnTrack)) {
            PSPAWNINFO *psTarget = NULL;
                  if (ppTarget) {
            psTarget = (PSPAWNINFO*)ppTarget;
                     *psTarget = pSpawnTrack;
                     szMsg[0] = 0;
                  }
                  else {
                     strcpy_s(szMsg, "Unable to target, address = 0");
                  }
               }
            }
            if (szMsg[0])
               if (!gFilterTarget)
                  WriteChatColor(szMsg, USERCOLOR_WHO);
         }
         }
      else if (pWnd == (CXWnd*)FilterRedButton || pWnd == (CXWnd*)FilterYellowButton
         || pWnd==(CXWnd*)FilterWhiteButton || pWnd==(CXWnd*)FilterBlueButton
         || pWnd==(CXWnd*)FilterLBlueButton || pWnd==(CXWnd*)FilterGreenButton || pWnd==(CXWnd*)FilterGrayButton
         || pWnd==(CXWnd*)TrackPlayersCombo || pWnd==(CXWnd*)TrackSortCombo) {
         if (Message == XWM_LCLICK)
            ReloadSpawn();
      }
      return CSidlScreenWnd::WndNotification(pWnd,Message,unknown);
   };

   CButtonWnd   *FilterRedButton;
   CButtonWnd   *FilterYellowButton;
   CButtonWnd   *FilterWhiteButton;
   CButtonWnd   *FilterBlueButton;
   CButtonWnd   *FilterLBlueButton;
   CButtonWnd   *FilterGreenButton;
   CButtonWnd   *FilterGrayButton;
   CListWnd   *TrackingList;
   CComboWnd   *TrackSortCombo;
   CComboWnd   *TrackPlayersCombo;
   CButtonWnd   *TrackButton;
   CButtonWnd   *DoneButton;
   CButtonWnd   *AutoUpdateButton;
};

CMyTrackingWnd *TrackingWnd=0;

// Called once, when the plugin is to initialize
PLUGIN_API VOID InitializePlugin(VOID)
{
   DebugSpewAlways("Initializing MQ2Tracking");
   AddCommand("/track",Track);
   lastRefresh = 0;
}

// Called once, when the plugin is to shutdown
PLUGIN_API VOID ShutdownPlugin(VOID)
{
   DebugSpewAlways("Shutting down MQ2Tracking");
   RemoveCommand("/track");
   DestroyTrackingWindow();
   if (pTrackSort)
   free(pTrackSort);   
}

PLUGIN_API VOID OnPulse(VOID)
{
   if (gGameState == GAMESTATE_INGAME && TrackingWnd) {
      pulsedelay++;
      if (pulsedelay>50) {
         pulsedelay = 0;
         if (pTrackSpawn)
            TrackSpawn();
      }
      if (TrackingWnd->AutoUpdateButton->Checked && ((clock() - lastRefresh) > (RefreshTimer * CLOCKS_PER_SEC))) {
         if (TrackingWnd) {
            if (TrackingWnd->IsVisible()) {
               ReloadSpawn();
         lastRefresh = clock();
      }
   }
}
   }
}

PLUGIN_API VOID OnRemoveSpawn(PSPAWNINFO pNewSpawn)
{
   if (gGameState == GAMESTATE_INGAME && pTrackSpawn)
      if (pNewSpawn == pTrackSpawn)
      StopTracking(NULL,NULL);
   }

PLUGIN_API VOID OnCleanUI(VOID)
{
   DebugSpewAlways("MQ2Tracking::OnCleanUI()");
   DestroyTrackingWindow();
}

PLUGIN_API VOID OnReloadUI(VOID)
{
   DebugSpewAlways("MQ2Tracking::OnReloadUI()");
   DestroyTrackingWindow();
}

PLUGIN_API VOID OnZoned()
{
   if (TrackingWnd)
      ReloadSpawn();
   lastRefresh = 0;
}

void CreateTrackingWindow()
{
   DebugSpewAlways("MQ2Tracking::CreateTrackingWindow()");
   if (TrackingWnd || !GetCharInfo() || !GetCharInfo()->pSpawn)
      return;
   if (pSidlMgr->FindScreenPieceTemplate("TrackingWnd")) {
      if (!TrackingWnd)
      TrackingWnd = new CMyTrackingWnd;
      TrackingWnd->DoneButton->CSetWindowText("Refresh");
      TrackingWnd->TrackSortCombo->DeleteAll();
      TrackingWnd->TrackSortCombo->SetColors(0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF);
      TrackingWnd->TrackSortCombo->InsertChoice("ID");
      TrackingWnd->TrackSortCombo->InsertChoice("Name");
      TrackingWnd->TrackSortCombo->InsertChoice("Level (Ascending)");
      TrackingWnd->TrackSortCombo->InsertChoice("Level (Descending)");
      TrackingWnd->TrackSortCombo->InsertChoice("Distance (Ascending)");
      TrackingWnd->TrackSortCombo->InsertChoice("Distance (Descending)");
      TrackingWnd->TrackSortCombo->SetChoice(1);
      TrackingWnd->TrackPlayersCombo->DeleteAll();
      TrackingWnd->TrackPlayersCombo->SetColors(0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF);
      TrackingWnd->TrackPlayersCombo->InsertChoice("All");
      TrackingWnd->TrackPlayersCombo->InsertChoice("PC");
      TrackingWnd->TrackPlayersCombo->InsertChoice("Group");
      TrackingWnd->TrackPlayersCombo->InsertChoice("NPC");
      TrackingWnd->TrackPlayersCombo->InsertChoice("Chest");
      TrackingWnd->TrackPlayersCombo->SetChoice(0);
      ReadWindowINI((PCSIDLWND)TrackingWnd);
      WriteWindowINI((PCSIDLWND)TrackingWnd);
      try {
         ((CXWnd*)TrackingWnd)->Show(0,0);
         ((CXWnd*)TrackingWnd)->Show(1,1);
      }
      catch(...) {
      }
   }
}

void DestroyTrackingWindow()
{
   DebugSpewAlways("MQ2Tracking::DestroyTrackingWindow()");
   if (TrackingWnd) {
      WriteWindowINI((PCSIDLWND)TrackingWnd);
      delete TrackingWnd;
      TrackingWnd=0;
   }
}

void ReadWindowINI(PCSIDLWND pWindow)
{
	CHAR Buffer[MAX_STRING] = { 0 };
	if (!pWindow)
		return;
	pWindow->SetLocation({ (LONG)GetPrivateProfileInt("Settings","ChatLeft",      164,INIFileName),
		(LONG)GetPrivateProfileInt("Settings","ChatTop",      357,INIFileName),
		(LONG)GetPrivateProfileInt("Settings","ChatRight",      375,INIFileName),
		(LONG)GetPrivateProfileInt("Settings","ChatBottom",      620,INIFileName) });

	pWindow->SetLocked((GetPrivateProfileInt("Settings", "Locked", 0, INIFileName) ? true : false));
	pWindow->SetFades((GetPrivateProfileInt("Settings", "Fades", 1, INIFileName) ? true : false));
	pWindow->SetFadeDelay(GetPrivateProfileInt("Settings", "Delay", 2000, INIFileName));
	pWindow->SetFadeDuration(GetPrivateProfileInt("Settings", "Duration", 500, INIFileName));
	pWindow->SetAlpha(GetPrivateProfileInt("Settings", "Alpha", 255, INIFileName));
	pWindow->SetFadeToAlpha(GetPrivateProfileInt("Settings", "FadeToAlpha", 255, INIFileName));
	pWindow->SetBGType(GetPrivateProfileInt("Settings", "BGType", 1, INIFileName));
	ARGBCOLOR col = { 0 };
	col.ARGB = pWindow->GetBGColor();
	col.A = GetPrivateProfileInt("Settings", "BGTint.alpha", 255, INIFileName);
	col.R = GetPrivateProfileInt("Settings", "BGTint.red", 0, INIFileName);
	col.G = GetPrivateProfileInt("Settings", "BGTint.green", 0, INIFileName);
	col.B = GetPrivateProfileInt("Settings", "BGTint.blue", 0, INIFileName);
	((CMyTrackingWnd*)pWindow)->FilterRedButton->SetCheck(1 & GetPrivateProfileInt("Filters", "ShowRed", 0, INIFileName));
	((CMyTrackingWnd*)pWindow)->FilterYellowButton->SetCheck(1 & GetPrivateProfileInt("Filters", "ShowYellow", 0, INIFileName));
	((CMyTrackingWnd*)pWindow)->FilterWhiteButton->SetCheck(1 & GetPrivateProfileInt("Filters", "ShowWhite", 0, INIFileName));
	((CMyTrackingWnd*)pWindow)->FilterBlueButton->SetCheck(1 & GetPrivateProfileInt("Filters", "ShowBlue", 0, INIFileName));
	((CMyTrackingWnd*)pWindow)->FilterLBlueButton->SetCheck(1 & GetPrivateProfileInt("Filters", "ShowLBlue", 0, INIFileName));
	((CMyTrackingWnd*)pWindow)->FilterGreenButton->SetCheck(1 & GetPrivateProfileInt("Filters", "ShowGreen", 0, INIFileName));
	((CMyTrackingWnd*)pWindow)->FilterGrayButton->SetCheck(1 & GetPrivateProfileInt("Filters", "ShowGray", 0, INIFileName));
	((CMyTrackingWnd*)pWindow)->TrackPlayersCombo->SetChoice(GetPrivateProfileInt("Filters", "Players", 0, INIFileName));
	((CMyTrackingWnd*)pWindow)->TrackSortCombo->SetChoice(GetPrivateProfileInt("Filters", "Sort", 0, INIFileName));
	TrackDist = GetPrivateProfileInt("Settings", "TrackDistance", 75, INIFileName);
	GetPrivateProfileString("Settings", "DisplayTpl", "[%l %C] %N (%R)", szNameTemplate, MAX_STRING, INIFileName);
	((CMyTrackingWnd*)pWindow)->AutoUpdateButton->SetCheck(1 & GetPrivateProfileInt("Settings", "AutoRefresh", 0, INIFileName));
	RefreshTimer = GetPrivateProfileInt("Settings", "RefreshDelay", 60, INIFileName);
}

void WriteWindowINI(PCSIDLWND pWindow)
{
   CHAR szTemp[MAX_STRING] = {0};
   if (!pWindow)
      return;
   if (pWindow->IsMinimized()) {
      _itoa_s(pWindow->GetOldLocation().top, szTemp, 10);
      WritePrivateProfileString("Settings", "ChatTop", szTemp, INIFileName);
      _itoa_s(pWindow->GetOldLocation().bottom, szTemp, 10);
      WritePrivateProfileString("Settings", "ChatBottom", szTemp, INIFileName);
      _itoa_s(pWindow->GetOldLocation().left, szTemp, 10);
      WritePrivateProfileString("Settings", "ChatLeft", szTemp, INIFileName);
      _itoa_s(pWindow->GetOldLocation().right, szTemp, 10);
      WritePrivateProfileString("Settings", "ChatRight", szTemp, INIFileName);
   }
   else {
      _itoa_s(pWindow->GetLocation().top, szTemp, 10);
      WritePrivateProfileString("Settings", "ChatTop", szTemp, INIFileName);
      _itoa_s(pWindow->GetLocation().bottom, szTemp, 10);
      WritePrivateProfileString("Settings", "ChatBottom", szTemp, INIFileName);
      _itoa_s(pWindow->GetLocation().left, szTemp, 10);
      WritePrivateProfileString("Settings", "ChatLeft", szTemp, INIFileName);
      _itoa_s(pWindow->GetLocation().right, szTemp, 10);
      WritePrivateProfileString("Settings", "ChatRight", szTemp, INIFileName);
   }
   _itoa_s(pWindow->IsLocked(), szTemp, 10);
   WritePrivateProfileString("Settings", "Locked", szTemp, INIFileName);
   _itoa_s(pWindow->GetFades(), szTemp, 10);
   WritePrivateProfileString("Settings", "Fades", szTemp, INIFileName);
   _itoa_s(pWindow->GetFadeDelay(), szTemp, 10);
   WritePrivateProfileString("Settings", "Delay", szTemp, INIFileName);
   _itoa_s(pWindow->GetFadeDuration(), szTemp, 10);
   WritePrivateProfileString("Settings", "Duration", szTemp, INIFileName);
   _itoa_s(pWindow->GetAlpha(), szTemp, 10);
   WritePrivateProfileString("Settings", "Alpha", szTemp, INIFileName);
   _itoa_s(pWindow->GetFadeToAlpha(), szTemp, 10);
   WritePrivateProfileString("Settings", "FadeToAlpha", szTemp, INIFileName);
   _itoa_s(pWindow->GetBGType(), szTemp, 10);
   WritePrivateProfileString("Settings", "BGType", szTemp, INIFileName);

	ARGBCOLOR col = { 0 };
	col.ARGB = pWindow->GetBGColor();
   _itoa_s(col.A, szTemp, 10);
   WritePrivateProfileString("Settings", "BGTint.alpha", szTemp, INIFileName);
   _itoa_s(col.R, szTemp, 10);
   WritePrivateProfileString("Settings", "BGTint.red", szTemp, INIFileName);
   _itoa_s(col.G, szTemp, 10);
   WritePrivateProfileString("Settings", "BGTint.green", szTemp, INIFileName);
   _itoa_s(col.B, szTemp, 10);
   WritePrivateProfileString("Settings", "BGTint.blue", szTemp, INIFileName);
   _itoa_s(1 & ((CMyTrackingWnd*)pWindow)->FilterRedButton->Checked, szTemp, 10);
   WritePrivateProfileString("Filters", "ShowRed", szTemp, INIFileName);
   _itoa_s(1 & ((CMyTrackingWnd*)pWindow)->FilterYellowButton->Checked, szTemp, 10);
   WritePrivateProfileString("Filters", "ShowYellow", szTemp, INIFileName);
   _itoa_s(1 & ((CMyTrackingWnd*)pWindow)->FilterWhiteButton->Checked, szTemp, 10);
   WritePrivateProfileString("Filters", "ShowWhite", szTemp, INIFileName);
   _itoa_s(1 & ((CMyTrackingWnd*)pWindow)->FilterBlueButton->Checked, szTemp, 10);
   WritePrivateProfileString("Filters", "ShowBlue", szTemp, INIFileName);
   _itoa_s(1 & ((CMyTrackingWnd*)pWindow)->FilterLBlueButton->Checked, szTemp, 10);
   WritePrivateProfileString("Filters", "ShowLBlue", szTemp, INIFileName);
   _itoa_s(1 & ((CMyTrackingWnd*)pWindow)->FilterGreenButton->Checked, szTemp, 10);
   WritePrivateProfileString("Filters", "ShowGreen", szTemp, INIFileName);
   _itoa_s(1 & ((CMyTrackingWnd*)pWindow)->FilterGrayButton->Checked, szTemp, 10);
   WritePrivateProfileString("Filters", "ShowGray", szTemp, INIFileName);
   _itoa_s(((CMyTrackingWnd*)pWindow)->TrackPlayersCombo->GetCurChoice(), szTemp, 10);
   WritePrivateProfileString("Filters", "Players", szTemp, INIFileName);
   _itoa_s(((CMyTrackingWnd*)pWindow)->TrackSortCombo->GetCurChoice(), szTemp, 10);
   WritePrivateProfileString("Filters", "Sort", szTemp, INIFileName);
   _itoa_s(TrackDist, szTemp, 10);
   WritePrivateProfileString("Settings", "TrackDistance", szTemp, INIFileName);
   WritePrivateProfileString("Settings","DisplayTpl",      szNameTemplate, INIFileName);
   _itoa_s(1 & ((CMyTrackingWnd*)pWindow)->AutoUpdateButton->Checked, szTemp, 10);
   WritePrivateProfileString("Settings", "AutoRefresh", szTemp, INIFileName);
   _itoa_s(RefreshTimer, szTemp, 10);
   WritePrivateProfileString("Settings", "RefreshDelay", szTemp, INIFileName);
}

DWORD STrackSortValue=0;
PSPAWNINFO SWhoSortOrigin=0;

static int pTrackSORTCompare(const void *A, const void *B)
{
   PSPAWNINFO SpawnA=*(PSPAWNINFO*)A;
   PSPAWNINFO SpawnB=*(PSPAWNINFO*)B;
   switch (STrackSortValue) {
   case 0:   // spawnid - ascending
      if (SpawnA->SpawnID>SpawnB->SpawnID)
         return 1;
      if (SpawnA->SpawnID<SpawnB->SpawnID)
         return -1;
      break;
   case 2://level - ascending
      if (SpawnA->Level>SpawnB->Level)
         return 1;
      if (SpawnA->Level<SpawnB->Level)
         return -1;
      break;
   case 3://level - descending
      if (SpawnA->Level>SpawnB->Level)
         return -1;
      if (SpawnA->Level<SpawnB->Level)
         return 1;
      break;
   case 4://distance - ascending
      {
         FLOAT DistA=GetDistance(SWhoSortOrigin,SpawnA);
         FLOAT DistB=GetDistance(SWhoSortOrigin,SpawnB);
         if (DistA>DistB)
            return 1;
         if (DistA<DistB)
            return -1;
      }
      break;
   case 5://distance - descending
      {
         FLOAT DistA=GetDistance(SWhoSortOrigin,SpawnA);
         FLOAT DistB=GetDistance(SWhoSortOrigin,SpawnB);
         if (DistA>DistB)
            return -1;
         if (DistA<DistB)
            return 1;
      }
      break;
   }
   return _stricmp(CleanupName(SpawnA->Name, MAX_STRING), CleanupName(SpawnB->Name, MAX_STRING));
}

VOID ReloadSpawn()
{
   if (!TrackingWnd)
      return;
   SEARCHSPAWN SearchSpawn;
   CIndex<PSPAWNINFO> SpawnSet;
   PSPAWNINFO pChar = (PSPAWNINFO)pCharSpawn;
   PSPAWNINFO pSpawn = (PSPAWNINFO)pSpawnList;
   PSPAWNINFO pOrigin=pChar;
   PSEARCHSPAWN pSearchSpawn = &SearchSpawn;
   DWORD TotalMatching=0;
   ClearSearchSpawn(pSearchSpawn);
   ParseSearchSpawn(szCustomSearch,pSearchSpawn);
   switch (TrackingWnd->TrackPlayersCombo->GetCurChoice()) {
      case 1:
         pSearchSpawn->SpawnType = PC;
         break;
      case 2:
         pSearchSpawn->bGroup = TRUE;
         break;
      case 3:
         pSearchSpawn->SpawnType = NPC;
         break;
      case 4:
         pSearchSpawn->SpawnType = CHEST;
         break;
      default:
         pSearchSpawn->SpawnType = NONE;
         break;
   }
   pSearchSpawn->SortBy = TrackingWnd->TrackSortCombo->GetCurChoice();
   while (pSpawn)
   {
      DWORD myColor = ConColor(pSpawn);
      if (SpawnMatchesSearch(pSearchSpawn,pOrigin,pSpawn) && (
         (myColor == CONCOLOR_RED      && TrackingWnd->FilterRedButton->Checked & 1) ||
         (myColor == CONCOLOR_YELLOW      && TrackingWnd->FilterYellowButton->Checked & 1) ||
         (myColor == CONCOLOR_WHITE      && TrackingWnd->FilterWhiteButton->Checked & 1) ||
         (myColor == CONCOLOR_BLUE      && TrackingWnd->FilterBlueButton->Checked & 1) ||
         (myColor == CONCOLOR_LIGHTBLUE   && TrackingWnd->FilterLBlueButton->Checked & 1) ||
         (myColor == CONCOLOR_GREEN      && TrackingWnd->FilterGreenButton->Checked & 1) ||
         (myColor == CONCOLOR_GREY      && TrackingWnd->FilterGrayButton->Checked & 1)
         )
         ) {
            TotalMatching++;
            SpawnSet+=pSpawn;
      }
      pSpawn=pSpawn->pNext;
   }
   if (TotalMatching)
   {
      if (TotalMatching>1)
      {
         // sort our list
         STrackSortValue=pSearchSpawn->SortBy;
         SWhoSortOrigin=pOrigin;
         qsort(&SpawnSet.List[0],TotalMatching,sizeof(PSPAWNINFO),pTrackSORTCompare);
      }
   }
   DWORD ListSel = TrackingWnd->TrackingList->GetCurSel();
   if (ListSel != -1) {
      ListSel = pTrackSort[TrackingWnd->TrackingList->GetCurSel()].SpawnID;
   }
   TrackingWnd->TrackingList->DeleteAll();
   if (pTrackSort)
   free(pTrackSort);   
   pTrackSort = (PTRACKSORT)malloc(sizeof(TRACKSORT)*TotalMatching);
   for (DWORD N=0 ; N < TotalMatching ; N++)
   {
      pTrackSort[N].SpawnID = SpawnSet[N]->SpawnID;
      strcpy_s(pTrackSort[N].Name, SpawnSet[N]->Name);
      DWORD myColor = ConColor((PSPAWNINFO)(SpawnSet[N]));
      TrackingWnd->TrackingList->AddString(
         GenerateSpawnName(SpawnSet[N], szNameTemplate),
         ConColorToARGB(myColor),0,0);

      if (ListSel == (SpawnSet[N]->SpawnID)) {
         TrackingWnd->TrackingList->SetCurSel(N);   
      }
   }
}

PCHAR GenerateSpawnName(PSPAWNINFO pSpawn, PCHAR NameString)
{
	CHAR Name[MAX_STRING] = { 0 };
	unsigned long outpos=0;
#define AddMyString(str) {strcpy_s(&Name[outpos],MAX_STRING,str);outpos+=strlen(&Name[outpos]);}
#define AddInt(yourint) {_itoa_s(yourint,&Name[outpos],MAX_STRING,10);outpos+=strlen(&Name[outpos]);}
#define AddFloat10th(yourfloat) {outpos+=sprintf_s(&Name[outpos],MAX_STRING,"%.1f",yourfloat);}
	for (unsigned long N = 0 ; NameString[N] ; N++)
   {
      if (NameString[N]=='%')
      {
         N++;
         switch(NameString[N])
         {
         case 'N':// cleaned up name
            strcpy_s(&Name[outpos],MAX_STRING,pSpawn->Name);
            CleanupName(&Name[outpos], MAX_STRING, FALSE);
            outpos+=strlen(&Name[outpos]);
            break;
         case 'n':// original name
            AddMyString(pSpawn->Name);
            break;
         case 'h':// current health %
            AddInt(pSpawn->HPCurrent);
            break;
         case 'i':
            AddInt(pSpawn->SpawnID);
            break;
         case 'x':
            AddFloat10th(pSpawn->X);
            break;
         case 'y':
            AddFloat10th(pSpawn->Y);
            break;
         case 'z':
            AddFloat10th(pSpawn->Z);
            break;
         case 'd':
            AddFloat10th(GetDistance(pSpawn->X,pSpawn->Y));
            break;
         case 'R':
            AddMyString(pEverQuest->GetRaceDesc(pSpawn->mActorClient.Race));
            break;
         case 'C':
            AddMyString(pEverQuest->GetClassDesc(pSpawn->mActorClient.Class));
            break;
         case 'c':
            AddMyString(pEverQuest->GetClassThreeLetterCode(pSpawn->mActorClient.Class));
            break;
         case 'l':
            AddInt(pSpawn->Level);
            break;
         case '%':
            Name[outpos++]=NameString[N];
            break;
         }
      }
      else
         Name[outpos++]=NameString[N];
   }
   Name[outpos]=0;
   PCHAR ret=(PCHAR)malloc(strlen(Name)+1);
   strcpy_s(ret, strlen(Name) + 1, Name);
   return ret;
}

VOID Track(PSPAWNINFO pChar, PCHAR szLine)
{
   if (!strcmp(szLine,"help")) {
      SyntaxError("Usage: /track [off | target | help | save]");
      SyntaxError("Usage: /track filter  [off | <search string>]");
      SyntaxError("Usage: /track players [all | pc | group | npc]");
      SyntaxError("Usage: /track refresh [on | off | toggle | <refresh>]");
      return;
   }
   if (!strcmp(szLine,"off")) {
      StopTracking(NULL,NULL);
      return;
   }
   if (!strcmp(szLine,"target")) {
      if ((PSPAWNINFO)pTarget) {
         pTrackSpawn = (PSPAWNINFO)pTarget;
         TrackSpawn();
         IsTracking = true;
         return;
      }
      else {
         WriteChatColor("Must have a target to track");
         return;
      }
   }
   CHAR szMsg[MAX_STRING]={0};
   CHAR Arg1[MAX_STRING] = {0};
   CHAR Arg2[MAX_STRING] = {0};
   GetArg(Arg1,szLine,1);
   if (!strcmp(Arg1,"refresh")) {
      GetArg(Arg2,szLine,2);
      if (!strcmp(Arg2,"off")) {
         sprintf_s(szMsg,"Tracking window refresh: off");
         TrackingWnd->AutoUpdateButton->SetCheck(false);
      }
      else if (!strcmp(Arg2, "on")) {
         sprintf_s(szMsg,"Tracking window refresh: on");
         TrackingWnd->AutoUpdateButton->SetCheck(true);
      }
      else if (!strcmp(Arg2, "toggle")) {
         if (TrackingWnd->AutoUpdateButton->Checked)
            TrackingWnd->AutoUpdateButton->SetCheck(false);
         else
            TrackingWnd->AutoUpdateButton->SetCheck(true);
         sprintf_s(szMsg, "Tracking window refresh: %s", TrackingWnd->AutoUpdateButton->Checked ? "on" : "off");
      }
      else {
         RefreshTimer = atoi(Arg2);
         if (RefreshTimer < 1)   RefreshTimer = 1;
         if (RefreshTimer > 1000) RefreshTimer = 1000;
         sprintf_s(szMsg,"Tracking window will refresh every %i second(s).", RefreshTimer);
      }
      WriteChatColor(szMsg,USERCOLOR_CHAT_CHANNEL);
      return;
   }
   else if (!strcmp(Arg1, "filter")) {
      PCHAR szRest = GetNextArg(szLine);
      if (!szRest[0])
      {
         sprintf_s(szMsg,"Current custom filter: %s",szCustomSearch);
         WriteChatColor(szMsg,USERCOLOR_DEFAULT);
         return;
      }

      if (!_stricmp(szRest,"off"))
         strcpy_s(szCustomSearch,"");
      else
         strcpy_s(szCustomSearch,szRest);
      WritePrivateProfileString("Settings","CustomFilter",szNameTemplate,INIFileName);
      ReloadSpawn();
      return;
   }
   else if (!strcmp(Arg1, "players")) {
      GetArg(Arg2,szLine,2);
      if (TrackingWnd)
         if (TrackingWnd->IsVisible())
            DestroyTrackingWindow();
         else {
            ((CXWnd*)TrackingWnd)->Show(1,1);
            ReloadSpawn();
         }
      else {
         CreateTrackingWindow();
         ReloadSpawn();
      }
      if (!strcmp(Arg2,"all")) {
         // Track all
         TrackingWnd->TrackPlayersCombo->SetChoice(0);
      }
      else if (!strcmp(Arg2, "pc")) {
         // Track PCs
         TrackingWnd->TrackPlayersCombo->SetChoice(1);
      }
      else if (!strcmp(Arg2, "group")) {
         // Track Group
         TrackingWnd->TrackPlayersCombo->SetChoice(2);
      }
      else if (!strcmp(Arg2, "npc")) {
         // Track NPC
         TrackingWnd->TrackPlayersCombo->SetChoice(3);
      }
      else {
         WriteChatColor("FORMAT: /track players <all/pc/group/npc>");   
         return;
      }
      ReloadSpawn();
      return;
   }
   else if (!strcmp(Arg1, "save")) {
      WriteWindowINI((PCSIDLWND)TrackingWnd);
      WriteChatf("\arMQ2Tracking\aw::\agOptions and window settings saved.");
      return;
   }
   if (TrackingWnd)
      if (TrackingWnd->IsVisible())
         DestroyTrackingWindow();
      else {
         ((CXWnd*)TrackingWnd)->Show(1,1);
         ReadWindowINI((PCSIDLWND)TrackingWnd);
         ReloadSpawn();
      }
   else {
      CreateTrackingWindow();
      ReadWindowINI((PCSIDLWND)TrackingWnd);
      ReloadSpawn();
   }
}

VOID TrackSpawn()
{
   CHAR szMsg[MAX_STRING]={0};
   CHAR szName[MAX_STRING]={0};
   PSPAWNINFO pChar = (PSPAWNINFO)pCharSpawn;
   if (!(pTrackSpawn)) {
      //        sprintf_s(szMsg,"There were no matches for id: %d", (PSPAWNINFO)(SpawnSet[TrackingWnd->TrackingList->GetCurSel()])->SpawnID);
   }
   else {
      if (DistanceToSpawn(pChar,pTrackSpawn) > TrackDist) {
         INT Angle = (INT)((atan2f(pChar->X - pTrackSpawn->X, pChar->Y - pTrackSpawn->Y) * 180.0f / PI + 360.0f) / 22.5f + 0.5f) % 16;
         INT Heading = (INT)((pChar->Heading / 32.0f) + 8.5f) % 16;
         INT Direction = (INT)((32 + (Angle - Heading))/2) % 8;
		 strcpy_s(szName, pTrackSpawn->Name);
         sprintf_s(szMsg, MAX_STRING, "'%s' is %s; %1.2f away.",
            CleanupName(szName, MAX_STRING, FALSE),
            szDirection[Direction],
            DistanceToSpawn(pChar,pTrackSpawn));
         if (TrackingWnd)
			 TrackingWnd->DoneButton->CSetWindowText("Cancel");
      }
      else {
		 strcpy_s(szName, pTrackSpawn->Name);
         sprintf_s(szMsg, MAX_STRING, "'%s' reached (within %i).  Tracking stopped.",
            CleanupName(szName, MAX_STRING, FALSE), TrackDist);
         StopTracking(NULL,NULL);
      }
   }
   WriteChatColor(szMsg,USERCOLOR_WHO);
}

VOID StopTracking(PSPAWNINFO pChar, PCHAR szLine)
{
   DebugSpewAlways("MQ2Tracking::StopTracking:1");
   if (TrackingWnd)
	   TrackingWnd->DoneButton->CSetWindowText("Refresh");
   DebugSpewAlways("MQ2Tracking::StopTracking:2");
   pTrackSpawn = NULL;
   DebugSpewAlways("MQ2Tracking::StopTracking:3");
   if (IsTracking) {
      WriteChatColor("Tracking Stopped.");   
      IsTracking = false;
   }
   DebugSpewAlways("MQ2Tracking::StopTracking:4");
}

VOID TrackNames(PSPAWNINFO pChar, PCHAR szLine)
{
   bRunNextCommand = TRUE;
   CHAR szOut[MAX_STRING]={0};
   if (!szLine[0])
   {
      sprintf_s(szOut,"Normal naming string: %s",szNameTemplate);
      WriteChatColor(szOut,USERCOLOR_DEFAULT);
      return;
   }
   if (!_stricmp(szLine,"reset"))
      strcpy_s(szNameTemplate,"[%l %C] %N (%R)");
   else
      strcpy_s(szNameTemplate,szLine);
   sprintf_s(szOut,"Normal naming string: %s",szNameTemplate);
   WriteChatColor(szOut,USERCOLOR_DEFAULT);
   SyntaxError("Usage: /tracknames [value|reset]");
   WritePrivateProfileString("Settings","DisplayTpl",szNameTemplate,INIFileName);
   ReloadSpawn();
}
