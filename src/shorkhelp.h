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

typedef struct {
    char *name;
    char *type;
    char *file;
} Licence;

typedef struct {
    char *command;
    char *path;
    char *type;
    char *package;
    char *category;
    int man;
    char *name;
    char *aliases;
    char *desc;
    char *licences;
} ProgramEntry;



#ifndef EMBEDDED
#define BUILD_REPORT_PATH       "/var/log/shork/build-report.log"
#endif
#define CSV_BUFFER              49152
#define INITIAL_CMD_STR         128
#define MAX_CMD_STR             2048
#define MAX_LICENCES            100
#define MAX_PROG_ENTRIES        400

extern char OS_NAME[128];
extern Licence LICENCES[MAX_LICENCES];
extern int LICENCES_NO;
extern ProgramEntry PROG_ENTRIES[MAX_PROG_ENTRIES];
extern int PROG_ENTRIES_NO;



int getIsPT1(void);
int getOSName(void);
int loadLicences(void);
int loadProgramEntries(void);

#ifndef EMBEDDED
void printGuideDiscoveringHardware(void);
void printGuideEmacsCheatsheet(void);
void printGuideGitCommands(void);
void printGuideTmuxCheatsheet(void);
#endif

void printIntro(void);
void printIntroPT1(void);
#ifndef EMBEDDED
void printIntroStarted(void);
#endif

void printCmdsProgs(void);
void printCmdsProgsAlpha(void);
void printCmdsProgsCats(void);
void printSoftwareLicence(int);
void printSoftwareProgOverview(int);
#ifndef EMBEDDED
void printSoftwareSHORKTAINMENT(void);
#endif
void printSoftwareSHORKUTILS(void);

#ifndef EMBEDDED
void printOtherReport(void);
#endif
void printOtherSupport(void);

void showCmdRefMenu(void);
void showCmdsProgsMenu(void);
void showHelp(void);
void showLicencesMenu(void);
void showMainMenu(void);

#endif
