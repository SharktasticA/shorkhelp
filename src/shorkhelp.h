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
    char *source;
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
void printCommands(void);
void printEmacsCheatsheet(void);
void printGitCommands(void);
void printIntro(void);
void printLicence(int i);
void printProgOverview(int i);
void printPT1(void);
void printSHORKEntertainment(void);
void printSHORKUtilities(void);
void printStarted(void);
void printTmuxCheatsheet(void);
void showCommandReference(void);
void showHelp(void);
void showLicencesMenu(void);
void showMainMenu(void);

#endif
