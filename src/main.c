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



#include "shorkhelp.h"
#include "shorkmenu.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>



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
        if (strcmp(argv[1], "--commands") == 0)
        {
            setupMenuSys();
            PROG_ENTRIES_NO = loadProgramEntries();
            clearScreen();
            printCommands();
            clearScreen();
        }
        else if (strcmp(argv[1], "--emacs") == 0 || strcmp(argv[1], "--mg") == 0)
        {
            setupMenuSys();
            clearScreen();
            printEmacsCheatsheet();
            clearScreen();
        }
        else if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)
            showHelp();
        else if (strcmp(argv[1], "--git") == 0)
        {
            setupMenuSys();
            clearScreen();
            printGitCommands();
            clearScreen();
        }
        else if (strcmp(argv[1], "--intro") == 0 && strncmp(OS_NAME, "SHORK 486", 9) == 0)
        {
            setupMenuSys();
            clearScreen();
            printIntro();
            clearScreen();
        }
        else if (strcmp(argv[1], "--pt1") == 0 && getIsPT1())
        {
            setupMenuSys();
            clearScreen();
            printPT1();
            clearScreen();
        }
        else if (strcmp(argv[1], "--shorktainment") == 0)
        {
            setupMenuSys();
            clearScreen();
            printSHORKEntertainment();
            clearScreen();
        }
        else if (strcmp(argv[1], "--shorkutils") == 0)
        {
            setupMenuSys();
            clearScreen();
            printSHORKUtilities();
            clearScreen();
        }
        else if (strcmp(argv[1], "--started") == 0)
        {
            setupMenuSys();
            clearScreen();
            printStarted();
            clearScreen();
        }
        else if (strcmp(argv[1], "--tmux") == 0)
        {
            setupMenuSys();
            clearScreen();
            printTmuxCheatsheet();
            clearScreen();
        }
        else if ((strcmp(argv[1], "-v") == 0) || (strcmp(argv[1], "--version") == 0))
            printf("SHORKHELP %s\n", VERSION);
        else
            showHelp();
    }

    return 0;
}
