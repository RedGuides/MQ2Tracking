// MQ2Tracking

#include <mq/Plugin.h>
#include <time.h>

PreSetup("MQ2Tracking");
PLUGIN_VERSION(1.0);

void CreateTrackingWindow();
void DestroyTrackingWindow();
std::string GenerateSpawnName(PSPAWNINFO pSpawn, const char* NameString);
void ReloadSpawn();
void Track(PSPAWNINFO pChar, PCHAR szLine);
void StopTracking();
void TrackSpawn();

bool IsTracking = false;
int pulsedelay = 0;
int TrackDist = 75;
int RefreshTimer = 60;
char szNameTemplate[MAX_STRING] = { 0 };
char szCustomSearch[MAX_STRING] = { 0 };
clock_t lastRefresh = clock();
PSPAWNINFO pTrackSpawn;

const char* szDirection[] = {
	"straight ahead",          //0
	"ahead and to the left",   //1
	"to the left",             //2
	"behind and to the left",  //3
	"directly behind",         //4
	"behind and to the right", //5
	"to the right",            //6
	"ahead and to the right"   //7
};

typedef struct TRACKSORT {
	char Name[64];
	int SpawnID;
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
	}

	int WndNotification(CXWnd *pWnd, unsigned int Message, void *unknown) {
		if (pWnd==0) {
			if (Message==XWM_CLOSE) {
				CreateTrackingWindow();
				this->Show(true,true);
				SetVisible(true);
				return 1;
			}
		}
		if (pWnd == (CXWnd*)TrackButton) {
			if (Message==XWM_LCLICK) {
				pTrackSpawn = GetSpawnByID(pTrackSort[TrackingList->GetCurSel()].SpawnID);
				if (pTrackSpawn) {
					WriteChatColor("Tracking started.");
					TrackSpawn();
				}
			}
		}
		else if (pWnd == (CXWnd*)DoneButton) {
			if (Message==XWM_LCLICK) {
				StopTracking();
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
				if (SPAWNINFO* pSpawnTrack = GetSpawnByID(pTrackSort[TrackingList->GetCurSel()].SpawnID)) {
					if (IsTargetable(pSpawnTrack)) {
						if (pTarget) {
							pTarget = pSpawnTrack;
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
	}

	CButtonWnd *FilterRedButton;
	CButtonWnd *FilterYellowButton;
	CButtonWnd *FilterWhiteButton;
	CButtonWnd *FilterBlueButton;
	CButtonWnd *FilterLBlueButton;
	CButtonWnd *FilterGreenButton;
	CButtonWnd *FilterGrayButton;
	CListWnd *TrackingList;
	CComboWnd *TrackSortCombo;
	CComboWnd *TrackPlayersCombo;
	CButtonWnd *TrackButton;
	CButtonWnd *DoneButton;
	CButtonWnd *AutoUpdateButton;
};

CMyTrackingWnd* TrackingWnd = nullptr;

PLUGIN_API void InitializePlugin()
{
	DebugSpewAlways("Initializing MQ2Tracking");
	AddCommand("/track",Track);
	lastRefresh = 0;
}

PLUGIN_API void ShutdownPlugin()
{
	DebugSpewAlways("Shutting down MQ2Tracking");
	RemoveCommand("/track");
	DestroyTrackingWindow();
	if (pTrackSort)
		free(pTrackSort);
}

PLUGIN_API void OnPulse()
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

PLUGIN_API void OnRemoveSpawn(PSPAWNINFO pSpawn)
{
	if (gGameState == GAMESTATE_INGAME && pTrackSpawn)
		if (pSpawn == pTrackSpawn)
			StopTracking();
}

PLUGIN_API void OnCleanUI()
{
	DebugSpewAlways("MQ2Tracking::OnCleanUI()");
	DestroyTrackingWindow();
}

PLUGIN_API void OnReloadUI()
{
	DebugSpewAlways("MQ2Tracking::OnReloadUI()");
	DestroyTrackingWindow();
}

PLUGIN_API void OnZoned()
{
	if (TrackingWnd)
		ReloadSpawn();
	lastRefresh = 0;
}

void ReadWindowINI(CMyTrackingWnd* pWindow)
{
	if (!pWindow)
		return;

	pWindow->SetLocation({ GetPrivateProfileInt("Settings","ChatLeft", 164,INIFileName),
		GetPrivateProfileInt("Settings","ChatTop", 357,INIFileName),
		GetPrivateProfileInt("Settings","ChatRight", 375,INIFileName),
		GetPrivateProfileInt("Settings","ChatBottom", 620,INIFileName) });

	pWindow->SetLocked(GetPrivateProfileBool("Settings", "Locked", false, INIFileName));
	pWindow->SetFades(GetPrivateProfileBool("Settings", "Fades", true, INIFileName));
	pWindow->SetFadeDelay(GetPrivateProfileInt("Settings", "Delay", 2000, INIFileName));
	pWindow->SetFadeDuration(GetPrivateProfileInt("Settings", "Duration", 500, INIFileName));
	pWindow->SetAlpha(GetPrivateProfileInt("Settings", "Alpha", 255, INIFileName));
	pWindow->SetFadeToAlpha(GetPrivateProfileInt("Settings", "FadeToAlpha", 255, INIFileName));
	pWindow->SetBGType(GetPrivateProfileInt("Settings", "BGType", 1, INIFileName));
	pWindow->SetBGColor(GetPrivateProfileColor("Settings", "BGTint", MQColor(0, 0, 0), INIFileName));
	pWindow->FilterRedButton->SetCheck(GetPrivateProfileBool("Filters", "ShowRed", false, INIFileName));
	pWindow->FilterYellowButton->SetCheck(GetPrivateProfileBool("Filters", "ShowYellow", false, INIFileName));
	pWindow->FilterWhiteButton->SetCheck(GetPrivateProfileBool("Filters", "ShowWhite", false, INIFileName));
	pWindow->FilterBlueButton->SetCheck(GetPrivateProfileBool("Filters", "ShowBlue", false, INIFileName));
	pWindow->FilterLBlueButton->SetCheck(GetPrivateProfileBool("Filters", "ShowLBlue", false, INIFileName));
	pWindow->FilterGreenButton->SetCheck(GetPrivateProfileBool("Filters", "ShowGreen", false, INIFileName));
	pWindow->FilterGrayButton->SetCheck(GetPrivateProfileBool("Filters", "ShowGray", false, INIFileName));
	pWindow->TrackPlayersCombo->SetChoice(GetPrivateProfileInt("Filters", "Players", 0, INIFileName));
	pWindow->TrackSortCombo->SetChoice(GetPrivateProfileInt("Filters", "Sort", 0, INIFileName));
	TrackDist = GetPrivateProfileInt("Settings", "TrackDistance", 75, INIFileName);
	GetPrivateProfileString("Settings", "DisplayTpl", "[%l %C] %N (%R)", szNameTemplate, MAX_STRING, INIFileName);
	pWindow->AutoUpdateButton->SetCheck(GetPrivateProfileBool("Settings", "AutoRefresh", false , INIFileName));
	RefreshTimer = GetPrivateProfileInt("Settings", "RefreshDelay", 60, INIFileName);
}

void WriteWindowINI(CMyTrackingWnd* pWindow)
{
	if (!pWindow)
		return;

	if (pWindow->IsMinimized()) {
		WritePrivateProfileInt("Settings", "ChatTop", pWindow->GetOldLocation().top, INIFileName);
		WritePrivateProfileInt("Settings", "ChatBottom", pWindow->GetOldLocation().bottom, INIFileName);
		WritePrivateProfileInt("Settings", "ChatLeft", pWindow->GetOldLocation().left, INIFileName);
		WritePrivateProfileInt("Settings", "ChatRight", pWindow->GetOldLocation().right, INIFileName);
	}
	else {
		WritePrivateProfileInt("Settings", "ChatTop", pWindow->GetLocation().top, INIFileName);
		WritePrivateProfileInt("Settings", "ChatBottom", pWindow->GetLocation().bottom, INIFileName);
		WritePrivateProfileInt("Settings", "ChatLeft", pWindow->GetLocation().left, INIFileName);
		WritePrivateProfileInt("Settings", "ChatRight", pWindow->GetLocation().right, INIFileName);
	}

	WritePrivateProfileBool("Settings", "Locked", pWindow->IsLocked(), INIFileName);
	WritePrivateProfileBool("Settings", "Fades", pWindow->GetFades(), INIFileName);
	WritePrivateProfileInt("Settings", "Delay", pWindow->GetFadeDelay(), INIFileName);
	WritePrivateProfileInt("Settings", "Duration", pWindow->GetFadeDuration(), INIFileName);
	WritePrivateProfileInt("Settings", "Alpha", pWindow->GetAlpha(), INIFileName);
	WritePrivateProfileInt("Settings", "FadeToAlpha", pWindow->GetFadeToAlpha(), INIFileName);
	WritePrivateProfileInt("Settings", "BGType", pWindow->GetBGType(), INIFileName);

	WritePrivateProfileColor("Settings", "BGTint", pWindow->GetBGColor(), INIFileName);
	WritePrivateProfileBool("Filters", "ShowRed", pWindow->FilterRedButton->Checked, INIFileName);
	WritePrivateProfileBool("Filters", "ShowYellow", pWindow->FilterYellowButton->Checked, INIFileName);
	WritePrivateProfileBool("Filters", "ShowWhite", pWindow->FilterWhiteButton->Checked, INIFileName);
	WritePrivateProfileBool("Filters", "ShowBlue", pWindow->FilterBlueButton->Checked, INIFileName);
	WritePrivateProfileBool("Filters", "ShowLBlue", pWindow->FilterLBlueButton->Checked, INIFileName);
	WritePrivateProfileBool("Filters", "ShowGreen", pWindow->FilterGreenButton->Checked, INIFileName);
	WritePrivateProfileBool("Filters", "ShowGray", pWindow->FilterGrayButton->Checked, INIFileName);
	WritePrivateProfileInt("Filters", "Players", pWindow->TrackPlayersCombo->GetCurChoice(), INIFileName);
	WritePrivateProfileInt("Filters", "Sort", pWindow->TrackSortCombo->GetCurChoice(), INIFileName);
	WritePrivateProfileInt("Settings", "TrackDistance", TrackDist, INIFileName);
	WritePrivateProfileString("Settings","DisplayTpl", szNameTemplate, INIFileName);
	WritePrivateProfileBool("Settings", "AutoRefresh", pWindow->AutoUpdateButton->Checked, INIFileName);
	WritePrivateProfileInt("Settings", "RefreshDelay", RefreshTimer, INIFileName);
}

void CreateTrackingWindow()
{
	DebugSpewAlways("MQ2Tracking::CreateTrackingWindow()");

	if (TrackingWnd || !GetCharInfo() || !GetCharInfo()->pSpawn)
		return;

	if (pSidlMgr->FindScreenPieceTemplate("TrackingWnd")) {
		if (!TrackingWnd)
			TrackingWnd = new CMyTrackingWnd;

		TrackingWnd->DoneButton->SetWindowText("Refresh");
		TrackingWnd->TrackSortCombo->DeleteAll();
		TrackingWnd->TrackSortCombo->SetColors(0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF);
		TrackingWnd->TrackSortCombo->InsertChoice("ID");                 // 0
		TrackingWnd->TrackSortCombo->InsertChoice("ID (Reverse)");       // 1
		TrackingWnd->TrackSortCombo->InsertChoice("Name");               // 2
		TrackingWnd->TrackSortCombo->InsertChoice("Name (Reverse)");     // 3
		TrackingWnd->TrackSortCombo->InsertChoice("Level");              // 4
		TrackingWnd->TrackSortCombo->InsertChoice("Level (Reverse)");    // 5
		TrackingWnd->TrackSortCombo->InsertChoice("Distance");           // 6
		TrackingWnd->TrackSortCombo->InsertChoice("Distance (Reverse)"); // 7
		TrackingWnd->TrackSortCombo->SetChoice(1);
		TrackingWnd->TrackPlayersCombo->DeleteAll();
		TrackingWnd->TrackPlayersCombo->SetColors(0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF);
		TrackingWnd->TrackPlayersCombo->InsertChoice("All");
		TrackingWnd->TrackPlayersCombo->InsertChoice("PC");
		TrackingWnd->TrackPlayersCombo->InsertChoice("Group");
		TrackingWnd->TrackPlayersCombo->InsertChoice("NPC");
		TrackingWnd->TrackPlayersCombo->InsertChoice("Chest");
		TrackingWnd->TrackPlayersCombo->SetChoice(0);
		ReadWindowINI(TrackingWnd);
		WriteWindowINI(TrackingWnd);
		try {
			TrackingWnd->Show(false,false);
			TrackingWnd->Show(true,true);
		}
		catch(...) {
		}
	}
}

void DestroyTrackingWindow()
{
	DebugSpewAlways("MQ2Tracking::DestroyTrackingWindow()");
	if (TrackingWnd) {
		WriteWindowINI(TrackingWnd);
		delete TrackingWnd;
		TrackingWnd = nullptr;
	}
}


int STrackSortValue = 6;

struct TrackSortCompare
{
	bool operator()(SPAWNINFO* SpawnA, SPAWNINFO* SpawnB) const
	{
		switch (STrackSortValue) {
		case 0:  // id
			return SpawnA->SpawnID < SpawnB->SpawnID;
		case 1:  // id - reverse
			return SpawnA->SpawnID > SpawnB->SpawnID;
		case 2:  // name
			return _stricmp(SpawnA->Name, SpawnB->Name) < 0;
		case 3:  // name reverse
			return _stricmp(SpawnA->Name, SpawnB->Name) > 0;
		case 4:  // level
			return SpawnA->Level < SpawnB->Level;
		case 5:  // level reverse
			return SpawnA->Level > SpawnB->Level;
		case 6:  //distance
			return GetDistanceSquared(pLocalPlayer, SpawnA) < GetDistanceSquared(pLocalPlayer, SpawnB);
		case 7:  //distance reverse
			return GetDistanceSquared(pLocalPlayer, SpawnA) < GetDistanceSquared(pLocalPlayer, SpawnB);
		default:
			return false;
		}
	}
};

void ReloadSpawn()
{
	if (!TrackingWnd)
		return;

	MQSpawnSearch SearchSpawn;
	std::vector<SPAWNINFO*> SpawnSet;
	PSPAWNINFO pSpawn = pSpawnList;
	MQSpawnSearch* pSearchSpawn = &SearchSpawn;
	ClearSearchSpawn(pSearchSpawn);
	ParseSearchSpawn(szCustomSearch,pSearchSpawn);
	switch (TrackingWnd->TrackPlayersCombo->GetCurChoice()) {
		case 1:
			pSearchSpawn->SpawnType = PC;
			break;
		case 2:
			pSearchSpawn->bGroup = true;
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
	//pSearchSpawn->SortBy = static_cast<SearchSortBy>(TrackingWnd->TrackSortCombo->GetCurChoice());
	while (pSpawn)
	{
		DWORD myColor = ConColor(pSpawn);
		if (SpawnMatchesSearch(pSearchSpawn, pLocalPlayer, pSpawn) && (
			(myColor == CONCOLOR_RED && TrackingWnd->FilterRedButton->Checked & 1) ||
			(myColor == CONCOLOR_YELLOW && TrackingWnd->FilterYellowButton->Checked & 1) ||
			(myColor == CONCOLOR_WHITE && TrackingWnd->FilterWhiteButton->Checked & 1) ||
			(myColor == CONCOLOR_BLUE && TrackingWnd->FilterBlueButton->Checked & 1) ||
			(myColor == CONCOLOR_LIGHTBLUE && TrackingWnd->FilterLBlueButton->Checked & 1) ||
			(myColor == CONCOLOR_GREEN && TrackingWnd->FilterGreenButton->Checked & 1) ||
			(myColor == CONCOLOR_GREY && TrackingWnd->FilterGrayButton->Checked & 1)
			)
			) {
				SpawnSet.emplace_back(pSpawn);
		}
		pSpawn = pSpawn->GetNext();
	}
	if (SpawnSet.size() > 1)
	{
		// sort our list
		STrackSortValue = TrackingWnd->TrackSortCombo->GetCurChoice();
		std::sort(SpawnSet.begin(), SpawnSet.end(), TrackSortCompare());
	}

	int ListSel = TrackingWnd->TrackingList->GetCurSel();
	if (ListSel != -1) {
		ListSel = pTrackSort[TrackingWnd->TrackingList->GetCurSel()].SpawnID;
	}

	TrackingWnd->TrackingList->DeleteAll();

	if (pTrackSort)
		free(pTrackSort);

	pTrackSort = (PTRACKSORT)malloc(sizeof(TRACKSORT)*SpawnSet.size());

	for (unsigned int N = 0 ; N < SpawnSet.size() ; ++N)
	{
		pTrackSort[N].SpawnID = SpawnSet[N]->SpawnID;
		strcpy_s(pTrackSort[N].Name, SpawnSet[N]->Name);
		const int myColor = ConColor((SpawnSet[N]));
		TrackingWnd->TrackingList->AddString(
			GenerateSpawnName(SpawnSet[N], szNameTemplate).c_str(),
			ConColorToARGB(myColor),0, nullptr);

		if (ListSel == (SpawnSet[N]->SpawnID)) {
			TrackingWnd->TrackingList->SetCurSel(N);
		}
	}
}

std::string GenerateSpawnName(PSPAWNINFO pSpawn, const char* NameString)
{
	std::string parsedString = NameString;
	if (pSpawn != nullptr)
	{
		const std::string trackingToken = "||MQ2TrackingToken||";
		// Previous code used %% as escape
		parsedString = replace(parsedString, "%%", trackingToken);
		// Parse the rest
		parsedString = replace(parsedString, "%N", CleanupName(pSpawn->Name, EQ_MAX_NAME, false));
		parsedString = replace(parsedString, "%n", pSpawn->Name);
		parsedString = replace(parsedString, "%h", std::to_string(pSpawn->HPCurrent));
		parsedString = replace(parsedString, "%i", std::to_string(pSpawn->SpawnID));
		parsedString = replace(parsedString, "%x", fmt::format("{:.1f}", pSpawn->X));
		parsedString = replace(parsedString, "%y", fmt::format("{:.1f}", pSpawn->Y));
		parsedString = replace(parsedString, "%z", fmt::format("{:.1f}", pSpawn->Z));
		parsedString = replace(parsedString, "%d", fmt::format("{:.1f}", GetDistance(pSpawn->X,pSpawn->Y)));
		parsedString = replace(parsedString, "%R", pEverQuest->GetRaceDesc(pSpawn->mActorClient.Race));
		parsedString = replace(parsedString, "%C", pEverQuest->GetClassDesc(pSpawn->mActorClient.Class));
		parsedString = replace(parsedString, "%l", std::to_string(pSpawn->Level));
		// Put a single % back where it was escaped
		parsedString = replace(parsedString, trackingToken, "%");
	}

	return parsedString;
}

void Track(PSPAWNINFO pChar, PCHAR szLine)
{
	if (!strcmp(szLine,"help")) {
		SyntaxError("Usage: /track [off | target | help | save]");
		SyntaxError("Usage: /track filter  [off | <search string>]");
		SyntaxError("Usage: /track players [all | pc | group | npc]");
		SyntaxError("Usage: /track refresh [on | off | toggle | <refresh>]");
		return;
	}
	if (!strcmp(szLine,"off")) {
		StopTracking();
		return;
	}
	if (!strcmp(szLine,"target")) {
		if (pTarget) {
			pTrackSpawn = pTarget;
			TrackSpawn();
			IsTracking = true;
		}
		else {
			WriteChatColor("Must have a target to track");
		}
		return;
	}
	char szMsg[MAX_STRING]={0};
	char Arg1[MAX_STRING] = {0};
	char Arg2[MAX_STRING] = {0};
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
			RefreshTimer = GetIntFromString(Arg2, 0);
			if (RefreshTimer < 1)
				RefreshTimer = 1;
			if (RefreshTimer > 1000)
				RefreshTimer = 1000;
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
		WriteWindowINI(TrackingWnd);
		WriteChatf("\arMQ2Tracking\aw::\agOptions and window settings saved.");
		return;
	}
	if (TrackingWnd) {
		if (TrackingWnd->IsVisible())
			DestroyTrackingWindow();
		else {
			TrackingWnd->Show(true,true);
			ReadWindowINI(TrackingWnd);
			ReloadSpawn();
		}
	}
	else {
		CreateTrackingWindow();
		ReadWindowINI(TrackingWnd);
		ReloadSpawn();
	}
}

void TrackSpawn()
{
	char szMsg[MAX_STRING]={0};
	char szName[MAX_STRING]={0};
	PSPAWNINFO pChar = pCharSpawn;
	if (!(pTrackSpawn)) {
		// sprintf_s(szMsg,"There were no matches for id: %d", (PSPAWNINFO)(SpawnSet[TrackingWnd->TrackingList->GetCurSel()])->SpawnID);
	}
	else {
		if (DistanceToSpawn(pChar,pTrackSpawn) > TrackDist) {
			int Angle = static_cast<int>((atan2f(pChar->X - pTrackSpawn->X, pChar->Y - pTrackSpawn->Y) * 180.0f / PI + 360.0f) / 22.5f + 0.5f) % 16;
			int Heading = static_cast<int>((pChar->Heading / 32.0f) + 8.5f) % 16;
			int Direction = ((32 + (Angle - Heading))/2) % 8;
			strcpy_s(szName, pTrackSpawn->Name);
			sprintf_s(szMsg, MAX_STRING, "'%s' is %s; %1.2f away.",
				CleanupName(szName, MAX_STRING, false),
				szDirection[Direction],
				DistanceToSpawn(pChar,pTrackSpawn));
			if (TrackingWnd)
				TrackingWnd->DoneButton->SetWindowText("Cancel");
		}
		else {
			strcpy_s(szName, pTrackSpawn->Name);
			sprintf_s(szMsg, MAX_STRING, "'%s' reached (within %i).  Tracking stopped.",
			CleanupName(szName, MAX_STRING, false), TrackDist);
			StopTracking();
		}
	}
	WriteChatColor(szMsg,USERCOLOR_WHO);
}

void StopTracking()
{
	DebugSpewAlways("MQ2Tracking::StopTracking:1");
	if (TrackingWnd)
		TrackingWnd->DoneButton->SetWindowText("Refresh");
	DebugSpewAlways("MQ2Tracking::StopTracking:2");
	pTrackSpawn = nullptr;
	DebugSpewAlways("MQ2Tracking::StopTracking:3");
	if (IsTracking) {
		WriteChatColor("Tracking Stopped.");
		IsTracking = false;
	}
	DebugSpewAlways("MQ2Tracking::StopTracking:4");
}

void TrackNames(PSPAWNINFO pChar, PCHAR szLine)
{
	bRunNextCommand = true;
	char szOut[MAX_STRING]={0};
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
