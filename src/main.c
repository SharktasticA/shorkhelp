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
    ARG_NONE,
    ARG_COMMANDS,
    ARG_EMACS,
    ARG_GIT,
    ARG_HARDWARE,
    ARG_HELP,
    ARG_INTRO,
    ARG_LICENCES,
    ARG_PT1,
    ARG_REPORT,
    ARG_SHORKTAINMENT,
    ARG_SHORKUTILS,
    ARG_STARTED,
    ARG_SUPPORT,
    ARG_TMUX,
    ARG_VERSION
} Argument;



#include "general.h"
#include "shorkhelp.h"
#include "shorkmenu.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>



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
        Argument opt = ARG_NONE;

        for (int i = 1; i < argc; i++)
        {
            if (strcmp(argv[i], "--commands") == 0 && fileExists("/usr/share/shorkhelp/programs.csv"))
                opt = ARG_COMMANDS;
            else if (strcmp(argv[i], "--emacs") == 0 || strcmp(argv[i], "--mg") == 0)
                opt = ARG_EMACS;
            else if (strcmp(argv[i], "--git") == 0)
                opt = ARG_GIT;
            else if (strcmp(argv[i], "--hardware") == 0)
                opt = ARG_HARDWARE;
            else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0)
                opt = ARG_HELP;
            else if (strcmp(argv[i], "--intro") == 0 && strncmp(OS_NAME, "SHORK 486", 9) == 0)
                opt = ARG_INTRO;
            else if (strcmp(argv[i], "--licences") == 0 && fileExists("/LICENCES/manifest.csv"))
                opt = ARG_LICENCES;
            else if ((strcmp(argv[i], "-nc") == 0) || (strcmp(argv[i], "--no-col") == 0))
                COL_ENABLED = 0;
            else if (strcmp(argv[i], "--pt1") == 0 && getIsPT1())
                opt = ARG_PT1;
            else if (strcmp(argv[i], "--report") == 0 && access(BUILD_REPORT_PATH, F_OK) == 0)
                opt = ARG_REPORT;
            else if (strcmp(argv[i], "--shorktainment") == 0)
                opt = ARG_SHORKTAINMENT;
            else if (strcmp(argv[i], "--shorkutils") == 0)
                opt = ARG_SHORKUTILS;
            else if (strcmp(argv[i], "--started") == 0)
                opt = ARG_STARTED;
            else if (strcmp(argv[i], "--support") == 0)
                opt = ARG_SUPPORT;
            else if (strcmp(argv[i], "--tmux") == 0)
                opt = ARG_TMUX;
            else if ((strcmp(argv[i], "-v") == 0) || (strcmp(argv[i], "--version") == 0))
                opt = ARG_VERSION;
        }

        if (opt == ARG_COMMANDS)
        {
            PROG_ENTRIES_NO = loadProgramEntries();
            if (PROG_ENTRIES_NO == -1)
            {
                printf("ERROR: could not load programs.csv\n");
                return 1;
            }
            setupMenuSys();
            printSoftwareCommands();
        }
        else if (opt == ARG_EMACS)
        {
            setupMenuSys();
            printGuideEmacsCheatsheet();
        }
        else if (opt == ARG_GIT)
        {
            setupMenuSys();
            printGuideGitCommands();
        }
        else if (opt == ARG_HARDWARE)
        {
            setupMenuSys();
            printGuideDiscoveringHardware();
        }
        else if (opt == ARG_HELP)
            showHelp();
        else if (opt == ARG_INTRO)
        {
            setupMenuSys();
            printIntro();
        }
        else if (opt == ARG_LICENCES)
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
        else if (opt == ARG_PT1)
        {
            setupMenuSys();
            printIntroPT1();
        }
        else if (opt == ARG_REPORT)
        {
            setupMenuSys();
            printOtherReport();
        }
        else if (opt == ARG_SHORKTAINMENT)
        {
            setupMenuSys();
            printSoftwareSHORKTAINMENT();
        }
        else if (opt == ARG_SHORKUTILS)
        {
            setupMenuSys();
            printSoftwareSHORKUTILS();
        }
        else if (opt == ARG_STARTED)
        {
            setupMenuSys();
            printIntroStarted();
        }
        else if (opt == ARG_SUPPORT)
        {
            setupMenuSys();
            printOtherSupport();
        }
        else if (opt == ARG_TMUX)
        {
            setupMenuSys();
            printGuideTmuxCheatsheet();
        }
        else if (opt == ARG_VERSION)
            printf("SHORKHELP %s\n", VERSION);
        else
            showMainMenu();
    }

    return 0;
}
