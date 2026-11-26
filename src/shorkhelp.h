/*
    ######################################################
    ##            SHORK UTILITY - SHORKHELP             ##
    ######################################################
    ## Main program logic                               ##
    ######################################################
    ## Licence: GNU GENERAL PUBLIC LICENSE Version 3    ##
    ######################################################
    ## Kali (links.sharktastica.co.uk)                  ##
    ######################################################
*/



#ifndef SHORKHELP
#define SHORKHELP

#ifndef SHORK_DISKETTE
typedef struct {
    char *name;
    char *type;
    char *file;
} Licence;
#endif

typedef struct {
    char *id;
    char *command;
    char *path;
    char *type;
    char *package;
    char *category;
    int man;
    char *name;
    char *aliases;
    char *licences;
    char *desc;
} ProgramEntry;



#ifndef SHORK_DISKETTE
#define BUILD_REPORT_PATH   "/var/log/shork/build-report.log"
#define MAX_LICENCES        150
#endif

#define CSV_BUFFER              36864
#define PROG_ENTRY_CMD_LEN      256
#define INITIAL_CMD_STR         128
#define MAX_CMD_STR             2048
#define MAX_PROG_ENTRIES        600

#ifndef SHORK_DISKETTE
extern Licence LICENCES[MAX_LICENCES];
extern int LICENCES_NO;
extern int PROG_DESCS_NO;
#endif

extern ProgramEntry PROG_ENTRIES[MAX_PROG_ENTRIES];
extern int PROG_ENTRIES_NO;
extern char OS_NAME[128];



#ifndef SHORK_DISKETTE
int loadLicences(void);
int loadProgramDescs(void);

void printGuideDiscoveringHardware(void);
void printGuideEmacsCheatsheet(void);
void printGuideGitCommands(void);
void printGuideTmuxCheatsheet(void);

void printIntroStarted(void);

void printSoftwareLicence(int);
void printSoftwareProgOverview(int);
void printSoftwareSHORKTAINMENT(void);

void printOtherReport(void);

void showCmdRefMenu(void);
void showLicencesMenu(void);
#endif



int getIsPT1(void);
ProgramEntry **getProgramsWithAliases(int*, int);
int getOSName(void);
int loadProgramEntries(void);
int progCmp(const void*, const void*);

void printIntro(void);
void printIntroPT1(void);

void printCmdsProgs(void);
void printCmdsProgsAlpha(void);
void printCmdsProgsCats(void);
void printSoftwareSHORKUTILS(void);

void printOtherSupport(void);

void showCmdsProgsMenu(void);
void showHelp(void);
void showMainMenu(void);

#endif
