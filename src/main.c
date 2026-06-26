/*
    ######################################################
    ##            SHORK UTILITY - SHORKHELP             ##
    ######################################################
    ## A utility for SHORK Operating Systems that       ##
    ## informs about their capabilities and provides    ##
    ## guidance                                         ##
    ######################################################
    ## Licence: GNU GENERAL PUBLIC LICENSE Version 3    ##
    ######################################################
    ## Kali (sharktastica.co.uk)                        ##
    ######################################################
*/



static const char *VERSION = "1.0-pt1";



typedef enum
{
    HELP_NONE,
    HELP_COMMANDS,
    HELP_EMACS,
    HELP_GIT,
    HELP_HELP,
    HELP_INTRO,
    HELP_LICENCES,
    HELP_PT1,
    HELP_SHORKTAINMENT,
    HELP_SHORKUTILS,
    HELP_STARTED,
    HELP_TMUX,
    HELP_VERSION
} HelpOption;



#include "general.h"
#include "shorkhelp.h"
#include "shorkmenu.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>



int LICENCES_NO = -1;
Licence LICENCES[MAX_LICENCES];
int PROG_ENTRIES_NO = -1;
ProgramEntry PROG_ENTRIES[MAX_PROG_ENTRIES];



int main(int argc, char *argv[])
{
    if (!getOSName())
    {
        printf("ERROR: Could not retrieve Linux distribution's name\n");
        return 1;
    }

    if (argc == 1)
        showMainMenu();
    else
    {
        HelpOption opt = HELP_NONE;

        for (int i = 1; i < argc; i++)
        {
            if (strcmp(argv[1], "--commands") == 0 && fileExists("/usr/share/shorkhelp/programs.csv"))
                opt = HELP_COMMANDS;
            else if (strcmp(argv[1], "--emacs") == 0 || strcmp(argv[1], "--mg") == 0)
                opt = HELP_EMACS;
            else if (strcmp(argv[1], "--git") == 0)
                opt = HELP_GIT;
            else if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)
                opt = HELP_HELP;
            else if (strcmp(argv[1], "--intro") == 0 && strncmp(OS_NAME, "SHORK 486", 9) == 0)
                opt = HELP_INTRO;
            else if (strcmp(argv[1], "--licences") == 0 && fileExists("/LICENCES/manifest.csv"))
                opt = HELP_LICENCES;
            else if ((strcmp(argv[1], "-nc") == 0) || (strcmp(argv[1], "--no-col") == 0))
                COL_ENABLED = 0;
            else if (strcmp(argv[1], "--pt1") == 0 && getIsPT1())
                opt = HELP_PT1;
            else if (strcmp(argv[1], "--shorktainment") == 0)
                opt = HELP_SHORKTAINMENT;
            else if (strcmp(argv[1], "--shorkutils") == 0)
                opt = HELP_SHORKUTILS;
            else if (strcmp(argv[1], "--started") == 0)
                opt = HELP_STARTED;
            else if (strcmp(argv[1], "--tmux") == 0)
                opt = HELP_TMUX;
            else if ((strcmp(argv[1], "-v") == 0) || (strcmp(argv[1], "--version") == 0))
                opt = HELP_VERSION;
        }

        if (opt == HELP_COMMANDS)
        {
            PROG_ENTRIES_NO = loadProgramEntries();
            if (PROG_ENTRIES_NO == -1)
            {
                printf("ERROR: could not load programs.csv\n");
                return 1;
            }
            setupMenuSys();
            printCommands();
        }
        else if (opt == HELP_EMACS)
        {
            setupMenuSys();
            printEmacsCheatsheet();
        }
        else if (opt == HELP_GIT)
        {
            setupMenuSys();
            printGitCommands();
        }
        else if (opt == HELP_HELP)
            showHelp();
        else if (opt == HELP_INTRO)
        {
            setupMenuSys();
            printIntro();
        }
        else if (opt == HELP_LICENCES)
        {
            LICENCES_NO = loadLicences();
            if (LICENCES_NO == -1)
            {
                printf("ERROR: could not load manifest.csv\n");
                return 1;
            }
            setupMenuSys();
            showLicencesMenu();
        }
        else if (opt == HELP_PT1)
        {
            setupMenuSys();
            printPT1();
        }
        else if (opt == HELP_SHORKTAINMENT)
        {
            setupMenuSys();
            printSHORKEntertainment();
        }
        else if (opt == HELP_SHORKUTILS)
        {
            setupMenuSys();
            printSHORKUtilities();
        }
        else if (opt == HELP_STARTED)
        {
            setupMenuSys();
            printStarted();
        }
        else if (opt == HELP_TMUX)
        {
            setupMenuSys();
            printTmuxCheatsheet();
        }
        else if (opt == HELP_VERSION)
            printf("SHORKHELP %s\n", VERSION);
        else
            showMainMenu();
    }

    return 0;
}
