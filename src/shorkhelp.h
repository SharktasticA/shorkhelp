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
#define MAX_PROG_ENTRIES        300

extern char OS_NAME[128];
static ProgramEntry PROG_ENTRIES[MAX_PROG_ENTRIES];
static int PROG_ENTRIES_NO = -1;



int getIsPT1(void);
int getOSName(void);
int loadProgramEntries(void);
void printCommands(void);
void printEmacsCheatsheet(void);
void printGitCommands(void);
void printIntro(void);
void printProgOverview(int i);
void printPT1(void);
void printSHORKEntertainment(void);
void printSHORKUtilities(void);
void printStarted(void);
void printTmuxCheatsheet(void);
void showCommandReference(void);
void showHelp(void);
void showMainMenu(void);

#endif
