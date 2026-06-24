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

    if (argc == 1 || argc > 2)
    {
        showMainMenu();
        return 0;
    }
    else if (argc == 2)
    {
        if (strcmp(argv[1], "--commands") == 0 && fileExists("/usr/share/shorkhelp/programs.csv"))
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
        else if (strcmp(argv[1], "--emacs") == 0 || strcmp(argv[1], "--mg") == 0)
        {
            setupMenuSys();
            printEmacsCheatsheet();
        }
        else if (strcmp(argv[1], "--git") == 0)
        {
            setupMenuSys();
            printGitCommands();
        }
        else if (strcmp(argv[1], "--intro") == 0 && strncmp(OS_NAME, "SHORK 486", 9) == 0)
        {
            setupMenuSys();
            printIntro();
        }
        if (strcmp(argv[1], "--licences") == 0 && fileExists("/LICENCES/manifest.csv"))
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
        else if (strcmp(argv[1], "--pt1") == 0 && getIsPT1())
        {
            setupMenuSys();
            printPT1();
        }
        else if (strcmp(argv[1], "--shorktainment") == 0)
        {
            setupMenuSys();
            printSHORKEntertainment();
        }
        else if (strcmp(argv[1], "--shorkutils") == 0)
        {
            setupMenuSys();
            printSHORKUtilities();
        }
        else if (strcmp(argv[1], "--started") == 0)
        {
            setupMenuSys();
            printStarted();
        }
        else if (strcmp(argv[1], "--tmux") == 0)
        {
            setupMenuSys();
            printTmuxCheatsheet();
        }
        else if ((strcmp(argv[1], "-v") == 0) || (strcmp(argv[1], "--version") == 0))
            printf("SHORKHELP %s\n", VERSION);
        else
            showHelp();
    }

    return 0;
}
