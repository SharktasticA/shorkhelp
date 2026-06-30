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
    char *type;
    char *package;
    char *category;
    int man;
    char *name;
    char *aliases;
    char *desc;
    char *licences;
} ProgramEntry;



#define CSV_BUFFER              32768
#define MAX_CMD_STR             2048
#define MAX_LICENCES            100
#define MAX_PROG_ENTRIES        300

extern char OS_NAME[128];
extern Licence LICENCES[MAX_LICENCES];
extern int LICENCES_NO;
extern ProgramEntry PROG_ENTRIES[MAX_PROG_ENTRIES];
extern int PROG_ENTRIES_NO;



int getIsPT1(void);
int getOSName(void);
int loadLicences(void);
int loadProgramEntries(void);

void printGuideDiscoveringHardware(void);
void printGuideEmacsCheatsheet(void);
void printGuideGitCommands(void);
void printGuideTmuxCheatsheet(void);

void printIntro(void);
void printIntroPT1(void);
void printIntroStarted(void);

void printSoftwareCommands(void);
void printSoftwareLicence(int);
void printSoftwareProgOverview(int);
void printSoftwareSHORKTAINMENT(void);
void printSoftwareSHORKUTILS(void);

void showCommandRefMenu(void);
void showHelp(void);
void showLicencesMenu(void);
void showMainMenu(void);

#endif
