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


#include "colours.h"
#include "general.h"
#include "shorkhelp.h"
#include "shorkmenu.h"

#include <sys/ioctl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>



char OS_NAME[128];



/**
 * @return 1 if host SHORK OS is a PT1 build; 0 if not
 */
int getIsPT1(void)
{
    FILE *stream = fopen("/etc/issue", "r");
    if (stream)
    {
        char buffer[128];
        int found = 0;
        while (fgets(buffer, 128, stream))
        {
            if (strstr(buffer, "PT1"))
            {
                fclose(stream);
                return 1;
            }
        }
        fclose(stream);
    }
    return 0;
}

/**
 * Populates the OS_NAME global variable with the host Linux distribution's
 * name.
 * @return 1 if OS name found; 0 if not
 */
int getOSName(void)
{
    OS_NAME[0] = '\0';

    // Try os-release
    FILE *fStream = fopen("/etc/os-release", "r");
    if (fStream)
    {
        char buffer[128];
        while (fgets(buffer, 128, fStream))
        {
            if (strncmp(buffer, "PRETTY_NAME=", 12) == 0)
            {
                char *extract = extractFromPoint(buffer, sizeof(OS_NAME), '=', 1);
                strncpy(OS_NAME, extract, sizeof(OS_NAME) - 1);
                OS_NAME[sizeof(OS_NAME) - 1] = '\0';
                free(extract);
                break;
            }
        }
        fclose(fStream);
    }

    // Try issue
    if (OS_NAME[0] == '\0')
    {
        fStream = fopen("/etc/issue", "r");
        if (fStream)
        {
            char buffer[128];
            if (fgets(buffer, 128, fStream))
            {
                size_t len = strlen(buffer);
                if (len > 0 && buffer[len - 1] == '\n') buffer[len - 1] = '\0';
                char *p = strchr(buffer, '\\');
                if (p) *p = '\0';
                strncpy(OS_NAME, buffer, sizeof(OS_NAME) - 1);
                OS_NAME[sizeof(OS_NAME) - 1] = '\0';
            }
            fclose(fStream);
        }
    }

    return OS_NAME[0] != '\0' ? 1 : 0;
}

/**
 * Loads the manifests.csv file into LICENCES.
 * @returns Number of licences loaded; -1 if error
 */
int loadLicences(void)
{
    // Load csv file
    FILE *stream;
    if (fileExists("/LICENCES/manifest.csv"))
        stream = fopen("/LICENCES/manifest.csv", "r");
    else
        return -1;

    // Load csv into buffer
    static char buffer[CSV_BUFFER];
    size_t n = fread(buffer, 1, sizeof(buffer) - 1, stream);
    fclose(stream);
    buffer[n] = '\0';

    char *p = buffer;

    // Skip header line
    while (*p && *p != '\n') p++;
    if (*p == '\n') p++;

    int i = 0;

    while (*p && i < MAX_LICENCES)
    {
        char *line = p;

        // Find end of line
        while (*p && *p != '\n') p++;
        if (*p == '\n')
        {
            *p = '\0';
            p++;
        }

        if (*line == '\0')
            continue;

        // Load line
        char *fields[3];
        int fieldCount = loadCSVLine(line, fields, 3);

        // Check if malformed line/parsing
        if (fieldCount < 3)
            continue;

        // TODO: test file exists

        // Input line into entries
        LICENCES[i].name = fields[0];
        LICENCES[i].type = fields[1];
        LICENCES[i].file = fields[2];

        i++;
    }

    return i;
}

/**
 * Loads the programs.csv file into PROG_ENTRIES.
 * @returns Number of program entries loaded; -1 if error
 */
int loadProgramEntries(void)
{
    // Load csv file
    FILE *stream;
    if (fileExists("/usr/share/shorkhelp/programs.csv"))
        stream = fopen("/usr/share/shorkhelp/programs.csv", "r");
    else
    {   
        char *binDir = getBinDir();
        if (!binDir) return -1;

        // Check if binary directory + filename are too large for combining
        if (strlen(binDir) + strlen("programs.csv") >= PATH_MAX)
            return -1;

        // Combine binary directory and filename
        char binCSVPath[PATH_MAX];
        snprintf(binCSVPath, PATH_MAX, "%sprograms.csv", binDir);

        // Try binary-based path
        if (fileExists(binCSVPath)) stream = fopen(binCSVPath, "r");
        else return -1;
    }

    // Load csv into buffer
    static char buffer[CSV_BUFFER];
    size_t n = fread(buffer, 1, sizeof(buffer) - 1, stream);
    fclose(stream);
    buffer[n] = '\0';

    char *p = buffer;

    // Skip header line
    while (*p && *p != '\n') p++;
    if (*p == '\n') p++;

    int i = 0;

    while (*p && i < MAX_PROG_ENTRIES)
    {
        char *line = p;

        // Find end of line
        while (*p && *p != '\n') p++;
        if (*p == '\n')
        {
            *p = '\0';
            p++;
        }

        if (*line == '\0')
            continue;

        // Load line
        char *fields[8];
        int fieldCount = loadCSVLine(line, fields, 8);

        // Check if malformed line/parsing
        if (fieldCount < 8)
            continue;

        int isOptional = atoi(fields[1]);

        if (!isOptional || (isOptional && isProgramInstalled(fields[0], 1)))
        {
            // If no program name was given, assume it is the same as the command
            if (!fields[4] || fields[4][0] == '\0') fields[4] = fields[0];

            // Input line into entries
            PROG_ENTRIES[i].command = fields[0];
            PROG_ENTRIES[i].source = fields[2];
            PROG_ENTRIES[i].category = fields[3];
            PROG_ENTRIES[i].name = fields[4];
            PROG_ENTRIES[i].aliases = fields[5];

            if(strchr(fields[6], '"') != NULL)
            {
                // Remove doubled double quotes
                char *tmp = findReplace(fields[6], strlen(fields[6]), "\"\"", "\"");

                size_t descLen = strlen(tmp);

                // Remove start double quote
                if (descLen > 0 && tmp[0] == '"')
                    memmove(tmp, tmp + 1, descLen);
                
                descLen = strlen(tmp);

                // Remove end double quote
                if (descLen > 0 && tmp[descLen - 1] == '"')
                    tmp[descLen - 1] = '\0';

                PROG_ENTRIES[i].desc = tmp;
            }
            else PROG_ENTRIES[i].desc = fields[6];
            
            PROG_ENTRIES[i].licences = fields[7];

            i++;
        }
    }

    return i;
}

void printCommands(void)
{
    char genStr[MAX_CMD_STR] = "\0";
    char devStr[MAX_CMD_STR] = "\0";
    char netStr[MAX_CMD_STR] = "\0";
    char sysStr[MAX_CMD_STR] = "\0";
    char usrStr[MAX_CMD_STR] = "\0";
    char funStr[MAX_CMD_STR] = "\0";
    char ustStr[MAX_CMD_STR] = "\0";
    int netEnabled = 0;
    int usrEnabled = 0;
    int funEnabled = 0;
    int ustEnabled = 0;

    for (int i = 0; i < PROG_ENTRIES_NO; i++)
    {
        // Make sure data is present/valid
        const char *cmd = PROG_ENTRIES[i].command;
        const char *cat = PROG_ENTRIES[i].category;
        if (!cmd || !cat) continue;

        // Select which string to append to
        char *targetStr = NULL;
        if (strcmp(cat, "gen") == 0) targetStr = genStr;
        else if (strcmp(cat, "dev") == 0) targetStr = devStr;
        else if (strcmp(cat, "net") == 0) 
        {
            targetStr = netStr;
            netEnabled = 1;
        }
        else if (strcmp(cat, "sys") == 0) targetStr = sysStr;
        else if (strcmp(cat, "usr") == 0) 
        {
            targetStr = usrStr;
            usrEnabled = 1;
        }
        else if (strcmp(cat, "fun") == 0) 
        {
            targetStr = funStr;
            funEnabled = 1;
        }
        else
        {
            targetStr = ustStr;
            ustEnabled = 1;
        }

        // Ensure new cmd can fit in the string
        size_t len = strlen(targetStr);
        size_t cmdLen = strlen(cmd);
        if (len > 0 && len + 2 < MAX_CMD_STR)
            strcat(targetStr, ", ");

        // Append new cmd
        if (len + 2 + cmdLen < MAX_CMD_STR)
            strcat(targetStr, cmd);
    }

    const int combinedSize = MAX_CMD_STR * 7;
    char combinedStr[combinedSize];
    combinedStr[0] = '\0';
    int pos = 0;

    pos += snprintf(combinedStr + pos, combinedSize - pos, "\033[%smGeneral\033[%sm\n%s\n\n", COL_FOR_HEADING, COL_FOR_WHITE, genStr);
    pos += snprintf(combinedStr + pos, combinedSize - pos, "\033[%smEditors & development tools\033[%sm\n%s\n\n", COL_FOR_HEADING, COL_FOR_WHITE, devStr);
    if (netEnabled) pos += snprintf(combinedStr + pos, combinedSize - pos, "\033[%smNetworking & remote access\033[%sm\n%s\n\n", COL_FOR_HEADING, COL_FOR_WHITE, netStr);
    pos += snprintf(combinedStr + pos, combinedSize - pos, "\033[%smSystem & processes\033[%sm\n%s\n", COL_FOR_HEADING, COL_FOR_WHITE, sysStr);
    if (usrEnabled) pos += snprintf(combinedStr + pos, combinedSize - pos, "\n\033[%smUser management\033[%sm\n%s\n", COL_FOR_HEADING, COL_FOR_WHITE, usrStr);
    if (funEnabled) pos += snprintf(combinedStr + pos, combinedSize - pos, "\n\033[%smEntertainment\033[%sm\n%s\n", COL_FOR_HEADING, COL_FOR_WHITE, funStr);
    if (ustEnabled) pos += snprintf(combinedStr + pos, combinedSize - pos, "\n\033[%smUnsorted\033[%sm\n%s\n", COL_FOR_HEADING, COL_FOR_WHITE, ustStr);

    int lines = formatNewLines(combinedStr, TERM_SIZE.ws_col, NULL, 1);
    printTextScreen("Commands & programs list", combinedStr, lines, 1);
}

void printEmacsCheatsheet(void)
{
    char emacsStr[1500];
    snprintf(emacsStr, 1500, 
"\033[%smC\033[%sm = Ctrl                          \033[%smM\033[%sm = Alt\n\n\
\033[%smExit\033[%sm                   \033[%smC\033[%sm-x \033[%smC\033[%sm-c    \033[%smOpen file/new buffer\033[%sm      \033[%smC\033[%sm-x \033[%smC\033[%sm-f\n\
\033[%smSave current buffer\033[%sm    \033[%smC\033[%sm-x \033[%smC\033[%sm-s    \033[%smSave all buffers\033[%sm          \033[%smC\033[%sm-x s\n\n\
\033[%smSplit window\033[%sm           \033[%smC\033[%sm-x 2      \033[%smEnlarge window\033[%sm            \033[%smC\033[%sm-x ^\n\
\033[%smSwitch windows\033[%sm         \033[%smC\033[%sm-x o      \033[%smClose current window\033[%sm      \033[%smC\033[%sm-x 0\n\
\033[%smClose other windows\033[%sm    \033[%smC\033[%sm-x 1\n\n\
\033[%smGo to line\033[%sm             \033[%smC\033[%sm-x g      \033[%smUndo\033[%sm                      \033[%smC\033[%sm-_\n\
\033[%smSearch (forwards)\033[%sm      \033[%smC\033[%sm-s        \033[%smSearch (backwards)\033[%sm        \033[%smC\033[%sm-r\n\
\033[%smFind and replace\033[%sm       \033[%smM\033[%sm-%%        \033[%smReformat (fit to line)\033[%sm    \033[%smM\033[%sm-q\n",
    COL_FOR_BOLD_RED, COL_FOR_WHITE, COL_FOR_BOLD_MAGENTA, COL_FOR_WHITE,
    COL_FOR_OL, COL_FOR_WHITE, COL_FOR_RED, COL_FOR_WHITE, COL_FOR_RED, COL_FOR_WHITE, COL_FOR_OL, COL_FOR_WHITE, COL_FOR_RED, COL_FOR_WHITE, COL_FOR_RED, COL_FOR_WHITE,
    COL_FOR_OL, COL_FOR_WHITE, COL_FOR_RED, COL_FOR_WHITE, COL_FOR_RED, COL_FOR_WHITE, COL_FOR_OL, COL_FOR_WHITE, COL_FOR_RED, COL_FOR_WHITE,
    COL_FOR_OL, COL_FOR_WHITE, COL_FOR_RED, COL_FOR_WHITE, COL_FOR_OL, COL_FOR_WHITE, COL_FOR_RED, COL_FOR_WHITE,
    COL_FOR_OL, COL_FOR_WHITE, COL_FOR_RED, COL_FOR_WHITE, COL_FOR_OL, COL_FOR_WHITE, COL_FOR_RED, COL_FOR_WHITE,
    COL_FOR_OL, COL_FOR_WHITE, COL_FOR_RED, COL_FOR_WHITE,
    COL_FOR_OL, COL_FOR_WHITE, COL_FOR_RED, COL_FOR_WHITE, COL_FOR_OL, COL_FOR_WHITE, COL_FOR_RED, COL_FOR_WHITE,
    COL_FOR_OL, COL_FOR_WHITE, COL_FOR_RED, COL_FOR_WHITE, COL_FOR_OL, COL_FOR_WHITE, COL_FOR_RED, COL_FOR_WHITE,
    COL_FOR_OL, COL_FOR_WHITE, COL_FOR_MAGENTA, COL_FOR_WHITE, COL_FOR_OL, COL_FOR_WHITE, COL_FOR_MAGENTA, COL_FOR_WHITE);

    int lines = formatNewLines(emacsStr, TERM_SIZE.ws_col, NULL, 1);
    printTextScreen("Emacs (Mg) cheatsheet", emacsStr, lines, 1);
}

void printGitCommands(void)
{
    char gitStr[500];
    snprintf(gitStr, 500, "add, blame, branch, checkout, cherry-pick, clean, clone, commit, config, diff, fetch, init, log, merge, mv, pull, push, rebase, reflog, remote, reset, restore, rm, show, stash, status, switch, tag\n");

    int lines = formatNewLines(gitStr, TERM_SIZE.ws_col, NULL, 1);
    printTextScreen("Supported Git commands", gitStr, lines, 1);
}

void printIntro(void)
{
    const int strSize = 4192;
    char introStr[strSize];
    int pos = 0;

    pos += snprintf(introStr + pos, strSize - pos, "SHORK 486 is a 32-bit Linux distribution for 486 and Pentium (P5) PCs! It focuses on being as lean and small as possible, whilst still providing a modern kernel, robust command set, custom utilities, and hand-picked modern software.\n\n");

    pos += snprintf(introStr + pos, strSize - pos, "\033[%smGoals\033[%sm\nBesides being something fun to try on old PCs, SHORK 486 was founded on the belief that old PCs can still be useful to the right people, retrocomputing and gaming usage aside. SHORK 486 can be useful for lightweight desktop usage, SSH terminal usage, distraction-free typewriting (\"writerdecks\"), embedded applications, demonstrative use in academia/education, and as a technical demonstration of what old PCs and modern software targeted at them can actually still do! If it can save just one more PC from the landfills, it has done its job!\n\n", COL_FOR_HEADING, COL_FOR_WHITE);

    pos += snprintf(introStr + pos, strSize - pos, "\033[%smWhat's special\033[%sm\nSHORK 486 is a modern and maintained Linux distribution that can run on a processor architecture from 1989. Depending on configuration, it only requires between 8 and 24MiB system memory whilst still packing a lot of functionality for its size. Due to various factors, making such a distribution is increasingly difficult in the 2020s. System requirements are ever-increasing, with even the otherwise excellent Micro Core and Tiny Core requiring at least 26-46MB RAM, putting them out of range of many early 486 systems. As of Linux kernel 7.1 and beyond, support for 486 processors and various ISA and PCMCIA networking hardware has been dropped, and 32-bit x86 support in general is currently dropped by most mainstream distributions. Given this situation, SHORK 486 will try to fill this niche of a ready-to-go Linux distribution for such PCs by sticking with a minimal-where-possible philosophy, customisability and restoring support for older hardware with newer Linux kernels!\n\n", COL_FOR_HEADING, COL_FOR_WHITE);

    pos += snprintf(introStr + pos, strSize - pos, "\033[%smArchitecture\033[%sm\nSHORK 486 is not GNU/Linux as you may be accustomed to. Its init system and most core utilities are provided by BusyBox, a single-binary application well known for embedded usage. As needed, some util-linux and individual utilities are used to fill holes in BusyBox\'s suite. The system is compiled with musl instead of glibc, producing smaller binaries that also use fewer resources. The closest well-known Linux distribution to SHORK 486 is Alpine Linux.\n\n", COL_FOR_HEADING, COL_FOR_WHITE);

    pos += snprintf(introStr + pos, strSize - pos, "\033[%smOrigin & inspirations\033[%sm\nIn December 2025, YouTuber Action Retro posted a video on FLOPPINUX, something that turned out to be a very accessible means for me to learn how to make a working Linux system. It foremost inspired me to chase my dream of building a viable modern Linux system for my old IBM ThinkPads. SHORK Mini (as it was originally known) began as an automated build script based on FLOPPINUX\'s build instructions, but adapted for producing fixed disk images instead of diskette images. After that, more Linux kernel and BusyBox features were enabled, and other software was compiled to help fulfil that dream. Other inspirations from similar efforts include Gray386linux and Ocawesome101\'s blog post on running Linux on a 486SX. Aspects of Alpine Linux and Tiny Core are also kept in mind.", COL_FOR_HEADING, COL_FOR_WHITE);

    int lines = formatNewLines(introStr, TERM_SIZE.ws_col, NULL, 0);
    printTextScreen("Introduction to SHORK 486", introStr, lines, 1);
}

void printLicence(int i)
{
    char msgTitle[80];
    snprintf(msgTitle, 80, "%s (%s)", LICENCES[i].name, LICENCES[i].type);

    char licencePath[PATH_MAX];
    snprintf(licencePath, PATH_MAX, "/LICENCES/%s", LICENCES[i].file);

    char msgBody[49152];
    FILE *stream = fopen(licencePath, "r");
    if (stream)
    {
        size_t bytesRead = fread(msgBody, 1, sizeof(msgBody) - 1, stream);
        fclose(stream);
        msgBody[bytesRead] = '\0';
    }
    else
        return;

    int lines = formatNewLines(msgBody, TERM_SIZE.ws_col, NULL, 0);
    printTextScreen(msgTitle, msgBody, lines, 1);
}

void printProgOverview(int i)
{
    const int strSize = 4096;
    char overviewStr[strSize];
    int pos = 0;

    // Name
    if (strcmp(PROG_ENTRIES[i].category, "shork") == 0)
        pos += snprintf(overviewStr + pos, strSize - pos, "\033[%sm%s\033[%sm\n\n", COL_FOR_SHORKUTIL, PROG_ENTRIES[i].name, COL_RESET);
    else
        pos += snprintf(overviewStr + pos, strSize - pos, "\033[%sm%s\033[%sm\n\n", COL_FOR_CODE, PROG_ENTRIES[i].name, COL_RESET);

    // Description
    if (PROG_ENTRIES[i].desc[0] != '\0')
        pos += snprintf(overviewStr + pos, strSize - pos, "%s\n\n", PROG_ENTRIES[i].desc);
    else 
        pos += snprintf(overviewStr + pos, strSize - pos, "TO BE COMPLETED\n\n");

    // Aliases, sources, category & licences
    if (PROG_ENTRIES[i].aliases[0] != '\0')
        pos += snprintf(overviewStr + pos, strSize - pos, "\033[%smAliases:\033[%sm  %s\n", COL_FOR_OL, COL_RESET, PROG_ENTRIES[i].aliases);

    const char *source = PROG_ENTRIES[i].source;
    if (strcmp(source, "busybox") == 0)
        source = "BusyBox";
    else if (strcmp(source, "util-linux") == 0)
        source = "util-linux";
    else if (strcmp(source, "shorkutil") == 0)
        source = "SHORK Utilities";
    else if (strcmp(source, "shorktainment") == 0)
        source = "SHORK Entertainment";
    else if (strcmp(source, "bundled") == 0)
        source = "bundled software";
    pos += snprintf(overviewStr + pos, strSize - pos, "\033[%smSource:\033[%sm   %s\n", COL_FOR_OL, COL_RESET, source);

    const char *category = PROG_ENTRIES[i].category;
    if (strcmp(category, "gen") == 0)
        category = "general";
    else if (strcmp(category, "dev") == 0)
        category = "editors & development tools";
    else if (strcmp(category, "net") == 0)
        category = "networking & remote access";
    else if (strcmp(category, "sys") == 0)
        category = "system & processes";
    else if (strcmp(category, "usr") == 0)
        category = "user management";
    else if (strcmp(category, "fun") == 0)
        category = "entertainment";
    else if (strcmp(category, "shork") == 0)
        category = "SHORK";
    pos += snprintf(overviewStr + pos, strSize - pos, "\033[%smCategory:\033[%sm %s\n", COL_FOR_OL, COL_RESET, category);

    pos += snprintf(overviewStr + pos, strSize - pos, "\033[%smLicences:\033[%sm %s\n", COL_FOR_OL, COL_RESET, PROG_ENTRIES[i].licences);

    int lines = formatNewLines(overviewStr, TERM_SIZE.ws_col, NULL, 1);
    printTextScreen(PROG_ENTRIES[i].command, overviewStr, lines, 1);
}

void printPT1(void)
{
    const int strSize = 3072;
    char pt1Str[strSize];
    int pos = 0;

    pos += snprintf(pt1Str + pos, strSize - pos, "You are running a pre-release build of SHORK 486 - Public Test 1 (PT1). This is the first published release of SHORK 486, intended for testing purposes only and to gauge wider opinion to help the project's progression.\n\n");

    pos += snprintf(pt1Str + pos, strSize - pos, "\033[%smPurpose\033[%sm\nSHORK 486's development has progressed far enough that it has achieved the author's initial objectives, and is now appropriate for the first round of public testing to help scope out what to tackle next. PT1 also coincides with Linux kernel 7.1's debut, the first version to remove 486 and other relevant vintage hardware support, which this project subsequently patches back in. It would be ideal to validate that these patches are working correctly.\n\n", COL_FOR_HEADING, COL_FOR_WHITE);

    pos += snprintf(pt1Str + pos, strSize - pos, "\033[%smLicence & disclaimers\033[%sm\nSHORK 486 is a non-commercial, free and open-source operating system licenced under GPLv3 that is provided \"as is\" without warranty of any kind. In no event shall the contributors be liable for any damages arising from use of this software. Additionally, SHORK 486 is a work in progress, and this is a pre-release build. It should not be used for serious, production, or mission-critical work.\n\n", COL_FOR_HEADING, COL_FOR_WHITE);

    pos += snprintf(pt1Str + pos, strSize - pos, "\033[%smSuggestions & feedback\033[%sm\nIdeas for what to add and change to SHORK 486 are welcome! These can be suggestions for new bundled programs, new or improved features, and advice on how the system is configured and built. Frank and critical feedback on anything regarding the project is also welcomed, though it is expected that it is respectful and understanding. However, please understand that otherwise well-intentioned and interesting suggestions may be considered beyond the scope of this project, as the aim is to keep the system as minimal as possible, but they will be remembered for future sister projects. Suggestions and feedback can be given on the SHORK 486 GitHub repository or via social media channels found on \033[%smlinks.sharktastica.co.uk\033[%sm.\n\n", COL_FOR_HEADING, COL_FOR_WHITE, COL_FOR_CODE, COL_FOR_WHITE);

    pos += snprintf(pt1Str + pos, strSize - pos, "\033[%smBug & error reporting\033[%sm\nSHORK 486 does not collect any telemetry nor report anything to anyone, instead relying on you, the tester, to report and provide evidence of any issues that may arise when using it. If you encounter any, please report them! It is a good measure to include a robust description of your hardware and what you were trying to do, whether you compiled it yourself or used published install media, a screenshot/photo of \033[%smshorkfetch\033[%sm's output, \033[%sm/proc/cpuinfo\033[%sm's contents, and \033[%smdmesg\033[%sm output. Reports can be given on the SHORK 486 GitHub repository.", COL_FOR_HEADING, COL_FOR_WHITE, COL_FOR_CODE, COL_FOR_WHITE, COL_FOR_CODE, COL_FOR_WHITE, COL_FOR_CODE, COL_FOR_WHITE);

    int lines = formatNewLines(pt1Str, TERM_SIZE.ws_col, NULL, 0);
    printTextScreen("Public Test 1", pt1Str, lines, 1);
}

void printSHORKEntertainment(void)
{
    const int strSize = 2000;
    char shorktainmentStr[strSize];
    int pos = 0;

    if (isProgramInstalled("sl", 1))
        pos += snprintf(shorktainmentStr + pos, strSize - pos, "\033[%smshorklocomotive\033[%sm\nA shark-themed take on the \033[%smsl\033[%sm command that kindly pokes fun at making typos when trying to type \033[%smls\033[%sm. A cute SHORK will swim across the screen.\n\n", COL_FOR_SHORKUTIL, COL_FOR_WHITE, COL_FOR_CODE, COL_FOR_WHITE, COL_FOR_CODE, COL_FOR_WHITE);

    if (isProgramInstalled("shorkmatrix", 1))
        pos += snprintf(shorktainmentStr + pos, strSize - pos, "\033[%smshorkmatrix\033[%sm\nA minimalist, blue-themed take on the \033[%smcmatrix\033[%sm \"digital rain\" vertical scrolling text screensaver. Inspired by the 1999 film \"The Matrix\", the \"droplets\" are lines of blue characters that fall from the top of the terminal to the bottom, and are occasionally broken up.\n\n", COL_FOR_SHORKUTIL, COL_FOR_WHITE, COL_FOR_CODE, COL_FOR_WHITE);

    if (isProgramInstalled("shorksay", 1))
        pos += snprintf(shorktainmentStr + pos, strSize - pos, "\033[%smshorksay\033[%sm\nA shark-themed take on the \033[%smcowsay\033[%sm command that outputs a cute SHORK with a speech bubble containing a message of your choice. The message can be manually supplied or can be provided from another program's standard output (ie, using a pipe).\n\n", COL_FOR_SHORKUTIL, COL_FOR_WHITE, COL_FOR_CODE, COL_FOR_WHITE);

    int lines = formatNewLines(shorktainmentStr, TERM_SIZE.ws_col, NULL, 1);
    printTextScreen("SHORK Entertainment", shorktainmentStr, lines, 1);
}

void printSHORKUtilities(void)
{
    const int strSize = 2000;
    char shorkutilStr[strSize];
    int pos = 0;

    if (isProgramInstalled("shorkdir", 1))
        pos += snprintf(shorkutilStr + pos, strSize - pos, "\033[%smshorkdir\033[%sm\nA terminal-based file browser. It can surf the file system with either Vim-like keybinds, WASD or arrow keys, open text-based files with an installed editor of your choice, and inspect a file (if \033[%smfile\033[%sm is installed).\n\n", COL_FOR_SHORKUTIL, COL_FOR_WHITE, COL_FOR_CODE, COL_FOR_WHITE);

    if (isProgramInstalled("shorkfetch", 1))
        pos += snprintf(shorkutilStr + pos, strSize - pos, "\033[%smshorkfetch\033[%sm\nA shark-themed Linux fetch tool designed with speed, consistent output, and vintage hardware support in mind. It displays basic system and environment information in a summarised format.\n\n", COL_FOR_SHORKUTIL, COL_FOR_WHITE);

    pos += snprintf(shorkutilStr + pos, strSize - pos, "\033[%smshorkhelp\033[%sm\nProvides help and reference information regarding SHORK 486 and included software and tools. It provides a command database and list, guides, and cheatsheets.\n\n", COL_FOR_SHORKUTIL, COL_FOR_WHITE);

    if (isProgramInstalled("shorkoff", 1))
        pos += snprintf(shorkutilStr + pos, strSize - pos, "\033[%smshorkoff\033[%sm\nA shutdown helper that syncs outstanding write cache and safely brings the system to a controlled halt before a manual power off to help prevent data corruption or loss. Similar to \033[%smpoweroff\033[%sm or \033[%smshutdown -h\033[%sm.\n\n", COL_FOR_SHORKUTIL, COL_FOR_WHITE, COL_FOR_CODE, COL_FOR_WHITE, COL_FOR_CODE, COL_FOR_WHITE);

    if (isProgramInstalled("shorkset", 1))
        pos += snprintf(shorkutilStr + pos, strSize - pos, "\033[%smshorkset\033[%sm\nA settings program for changing SHORK 486's display resolution, keyboard layout (keymap), terminal PSF font, and terminal font colour.\n\n", COL_FOR_SHORKUTIL, COL_FOR_WHITE);

    int lines = formatNewLines(shorkutilStr, TERM_SIZE.ws_col, NULL, 1);
    printTextScreen("SHORK Utilities", shorkutilStr, lines, 1);
}

void printStarted(void)
{
    char gettingStartedStr[1200];
    snprintf(gettingStartedStr, 1200,
"\033[%sm1.\033[%sm Set your keyboard's layout with \033[%smshorkset\033[%sm.\n\n\
\033[%sm2.\033[%sm Pick a font and colour for the console terminal with \033[%smshorkset\033[%sm.\n\n\
\033[%sm3.\033[%sm Change your display's resolution with \033[%smshorkset\033[%sm. A reboot will be required.\n\n\
\033[%sm4.\033[%sm Set your computer's name by editing \033[%sm/etc/hostname\033[%sm. A reboot will be required, or you can run: \033[%smhostname \"$(cat /etc/hostname)\"\n\n\
\033[%sm5.\033[%sm (If applicable) Test your network connection with \033[%smping\033[%sm. For example: \033[%smping sharktastica.co.uk\n\n\
\033[%sm6.\033[%sm Run \033[%smshorkfetch\033[%sm to see a quick overview of your system and environment.\n\n\
\033[%sm7.\033[%sm Check \033[%smshorkhelp\033[%sm's other options to learn what commands and software are available, and see what guides may be of use.\n\n\
\033[%sm8.\033[%sm Use Ctrl+Alt+F1/F2/F3 or \033[%smchvt\033[%sm to switch between the three available virtual consoles, useful for multitasking, troubleshooting and recovery.\n\n\
\033[%sm9.\033[%sm When you are finished using SHORK 486, run \033[%smshorkoff\033[%sm to safely halt the system before powering off.\n",
    COL_FOR_OL, COL_FOR_WHITE, COL_FOR_SHORKUTIL, COL_FOR_WHITE,
    COL_FOR_OL, COL_FOR_WHITE, COL_FOR_SHORKUTIL, COL_FOR_WHITE,
    COL_FOR_OL, COL_FOR_WHITE, COL_FOR_SHORKUTIL, COL_FOR_WHITE,
    COL_FOR_OL, COL_FOR_WHITE, COL_FOR_CODE, COL_FOR_WHITE, COL_FOR_CODE,
    COL_FOR_OL, COL_FOR_WHITE, COL_FOR_CODE, COL_FOR_WHITE, COL_FOR_CODE,
    COL_FOR_OL, COL_FOR_WHITE, COL_FOR_SHORKUTIL, COL_FOR_WHITE,
    COL_FOR_OL, COL_FOR_WHITE, COL_FOR_SHORKUTIL, COL_FOR_WHITE,
    COL_FOR_OL, COL_FOR_WHITE, COL_FOR_CODE, COL_FOR_WHITE,
    COL_FOR_OL, COL_FOR_WHITE, COL_FOR_SHORKUTIL, COL_FOR_WHITE);

    int lines = formatNewLines(gettingStartedStr, TERM_SIZE.ws_col, "   ", 1);
    printTextScreen("Getting started", gettingStartedStr, lines, 1);
}

void printTmuxCheatsheet(void)
{
    char tmuxStr[1500];
    snprintf(tmuxStr, 1500, 
"\033[%smC\033[%sm = Ctrl\n\n\
\033[%smDetach session\033[%sm     \033[%smC\033[%sm-b d        \033[%smAccess tmux prompt\033[%sm    \033[%smC\033[%sm-b :\n\n\
\033[%smNew window\033[%sm         \033[%smC\033[%sm-b c        \033[%smClose window\033[%sm          \033[%smC\033[%sm-b &\n\
\033[%smList windows\033[%sm       \033[%smC\033[%sm-b w        \033[%smRename window\033[%sm         \033[%smC\033[%sm-b ,\n\
\033[%smPrevious window\033[%sm    \033[%smC\033[%sm-b p        \033[%smNext window\033[%sm           \033[%smC\033[%sm-b n\n\
\033[%smChange window\033[%sm      \033[%smC\033[%sm-b 0-9\n\n\
\033[%smVertical split\033[%sm     \033[%smC\033[%sm-b %%        \033[%smHorizontal split\033[%sm      \033[%smC\033[%sm-b \"\n\
\033[%smClose pane\033[%sm         \033[%smC\033[%sm-b x        \033[%smShow pane numbers\033[%sm     \033[%smC\033[%sm-b q\n\
\033[%smChange pane\033[%sm        \033[%smC\033[%sm-b q 0-9    \033[%smCycle panes\033[%sm           \033[%smC\033[%sm-b o\n\
\033[%smPane to window\033[%sm     \033[%smC\033[%sm-b !        \033[%smToggle pane zoom\033[%sm      \033[%smC\033[%sm-b z\n",
    COL_FOR_BOLD_RED, COL_FOR_WHITE,
    COL_FOR_OL, COL_FOR_WHITE, COL_FOR_RED, COL_FOR_WHITE, COL_FOR_OL, COL_FOR_WHITE, COL_FOR_RED, COL_FOR_WHITE,
    COL_FOR_OL, COL_FOR_WHITE, COL_FOR_RED, COL_FOR_WHITE, COL_FOR_OL, COL_FOR_WHITE, COL_FOR_RED, COL_FOR_WHITE,
    COL_FOR_OL, COL_FOR_WHITE, COL_FOR_RED, COL_FOR_WHITE, COL_FOR_OL, COL_FOR_WHITE, COL_FOR_RED, COL_FOR_WHITE,
    COL_FOR_OL, COL_FOR_WHITE, COL_FOR_RED, COL_FOR_WHITE, COL_FOR_OL, COL_FOR_WHITE, COL_FOR_RED, COL_FOR_WHITE,
    COL_FOR_OL, COL_FOR_WHITE, COL_FOR_RED, COL_FOR_WHITE,
    COL_FOR_OL, COL_FOR_WHITE, COL_FOR_RED, COL_FOR_WHITE, COL_FOR_OL, COL_FOR_WHITE, COL_FOR_RED, COL_FOR_WHITE,
    COL_FOR_OL, COL_FOR_WHITE, COL_FOR_RED, COL_FOR_WHITE, COL_FOR_OL, COL_FOR_WHITE, COL_FOR_RED, COL_FOR_WHITE,
    COL_FOR_OL, COL_FOR_WHITE, COL_FOR_RED, COL_FOR_WHITE, COL_FOR_OL, COL_FOR_WHITE, COL_FOR_RED, COL_FOR_WHITE,
    COL_FOR_OL, COL_FOR_WHITE, COL_FOR_RED, COL_FOR_WHITE, COL_FOR_OL, COL_FOR_WHITE, COL_FOR_RED, COL_FOR_WHITE);

    int lines = formatNewLines(tmuxStr, TERM_SIZE.ws_col, NULL, 1);
    printTextScreen("tmux cheatsheet", tmuxStr, lines, 1);
}

/**
 * Runs the command reference interface.
 */
void showCommandReference(void)
{
    // Create a menu containing found program entries
    MenuItem menu[PROG_ENTRIES_NO];
    for (int i = 0; i < PROG_ENTRIES_NO; i++)
    {
        snprintf(menu[i].id, sizeof(menu[i].id), "%s", PROG_ENTRIES[i].command);
        snprintf(menu[i].name, sizeof(menu[i].name), "%s", PROG_ENTRIES[i].command);
        menu[i].action = NULL;
        menu[i].isVisible = 1;
        menu[i].isStatic = 0;
    }



    // Prepare for multi-column menu
    int colWidth = 16;
    int cols = TERM_SIZE.ws_col / (colWidth + 3);
    if (cols < 1) cols = 1;
    if (cols > PROG_ENTRIES_NO) cols = PROG_ENTRIES_NO;
    int rows = (PROG_ENTRIES_NO + cols - 1) / cols;



    int running = 1;
    int cursorX = 1;
    int cursorY = 1;
    int cursorXPrev = 0;
    int cursorYPrev = 0;
    int maxY = 0;
    int fullRedraw = 1;

    while (running)
    {
        if (fullRedraw)
        {
            clearScreen();
            printHeader("Command reference (WIP)");
            printMenu(menu, PROG_ENTRIES_NO, NULL, cols, colWidth, rows, &cursorX, &cursorY, &cursorXPrev, &cursorYPrev);
            printFooter("[hjkl] Navigate [Enter] Select [q] Back");
        }
        else
        {
            if (COL_ENABLED)
                printf("\x1b[2;1H");
            else
                printf("\x1b[3;1H");
            printMenu(menu, PROG_ENTRIES_NO, NULL, cols, colWidth, rows, &cursorX, &cursorY, &cursorXPrev, &cursorYPrev);
        }

        NavInput input = getNavInput();

        fullRedraw = 1;
        cursorXPrev = cursorYPrev = 0;
        switch (input)
        {
            case CURSOR_LEFT:
                cursorXPrev = cursorX;
                cursorYPrev = cursorY;
                cursorX--;

                if (cursorX < 1) cursorX = cols;
                while ((maxY = rowsInCol(PROG_ENTRIES_NO, rows, cursorX)) == 0)
                {
                    cursorX--;
                    if (cursorX < 1) cursorX = cols;
                }
                if (cursorY > maxY) cursorY = maxY;
                if (cursorY < 1) cursorY = maxY;

                fullRedraw = 0;
                break;

            case CURSOR_RIGHT:
                cursorXPrev = cursorX;
                cursorYPrev = cursorY;
                cursorX++;

                if (cursorX > cols) cursorX = 1;
                while ((maxY = rowsInCol(PROG_ENTRIES_NO, rows, cursorX)) == 0)
                {
                    cursorX++;
                    if (cursorX > cols) cursorX = 1;
                }
                if (cursorY > maxY) cursorY = maxY;
                if (cursorY < 1) cursorY = maxY;

                fullRedraw = 0;
                break;

            case CURSOR_UP:
                cursorXPrev = cursorX;
                cursorYPrev = cursorY;
                cursorY--;

                if (cursorY < 1)
                    cursorY = rowsInCol(PROG_ENTRIES_NO, rows, cursorX);

                fullRedraw = 0;
                break;

            case CURSOR_DOWN:
                cursorXPrev = cursorX;
                cursorYPrev = cursorY;
                cursorY++;

                if (cursorY > rowsInCol(PROG_ENTRIES_NO, rows, cursorX))
                    cursorY = 1;
                    
                fullRedraw = 0;
                break;

            case ENTER:
                clearScreen();
                printProgOverview((cursorY - 1) + (cursorX - 1) * rows);
                break;
        
            case QUIT:
                running = 0;
                break;

            case INVALID:
                fullRedraw = 0;
                break;
        }
    }

    clearScreen();
}

/**
 * Displays help information.
 */
void showHelp(void)
{
    char desc[120] = "Displays help and reference information for using SHORK 486 and its included software and tools.\n";
    formatNewLines(desc, TERM_SIZE.ws_col, NULL, 0);
    printf("%s\n", desc);

    char usage[60] = "Usage: shorkhelp [OPTIONS]\n\n";
    formatNewLines(usage, TERM_SIZE.ws_col, NULL, 0);
    printf("%s", usage);

    char options[20] = "Options:\n";
    formatNewLines(options, TERM_SIZE.ws_col, NULL, 0);
    printf("%s", options);

    char commands[100] = "--commands       Displays commands & programs list and exit\n";
    formatNewLines(commands, TERM_SIZE.ws_col, "                 ", 0);
    printf("%s", commands);

    if (isProgramInstalled("emacs", 1) || isProgramInstalled("mg", 1))
    {
        char emacs[100] = "--emacs          Displays Emacs (Mg) cheatsheet and exit\n";
        formatNewLines(emacs, TERM_SIZE.ws_col, "                 ", 0);
        printf("%s", emacs);
    }

    char help[100] = "-h, --help       Displays help information and exits\n";
    formatNewLines(help, TERM_SIZE.ws_col, "                 ", 0);
    printf("%s", help);

    if (isProgramInstalled("git", 1))
    {
        char git[100] = "--git            Displays supported Git commands and exits\n";
        formatNewLines(git, TERM_SIZE.ws_col, "                 ", 0);
        printf("%s", git);
    }

    char intro[100] = "--intro          Displays introduction to SHORK 486 and exit\n";
    formatNewLines(intro, TERM_SIZE.ws_col, "                 ", 0);
    printf("%s", intro);

    char licences[100] = "--licences       Displays licences menu\n";
    formatNewLines(licences, TERM_SIZE.ws_col, "                 ", 0);
    printf("%s", licences);

    if (getIsPT1())
    {
        char pt1[80] = "--pt1            Displays Public Test 1 information and exits\n";
        formatNewLines(pt1, TERM_SIZE.ws_col, "                 ", 0);
        printf("%s", pt1);
    }

    if (isProgramInstalled("sl", 1) || isProgramInstalled("shorkmatrix", 1) || isProgramInstalled("shorksay", 1))
    {
        char shorktainment[100] = "--shorktainment  Displays SHORK Entertainment list and exit\n";
        formatNewLines(shorktainment, TERM_SIZE.ws_col, "                 ", 0);
        printf("%s", shorktainment);
    }

    char shorkutils[100] = "--shorkutils     Displays SHORK Utilities list and exit\n";
    formatNewLines(shorkutils, TERM_SIZE.ws_col, "                 ", 0);
    printf("%s", shorkutils);

    if (strncmp(OS_NAME, "SHORK 486", 9) == 0)
    {
        char started[100] = "--started        Displays getting started guide and exit\n";
        formatNewLines(started, TERM_SIZE.ws_col, "                 ", 0);
        printf("%s", started);
    }

    if (isProgramInstalled("tmux", 1))
    {
        char tmux[100] = "--tmux           Displays tmux cheatsheet and exit\n";
        formatNewLines(tmux, TERM_SIZE.ws_col, "                 ", 0);
        printf("%s", tmux);
    }

    char version[100] = "-v, --version    Displays version number and exits\n";
    formatNewLines(version, TERM_SIZE.ws_col, "                 ", 0);
    printf("%s", version);
}

/**
 * Runs the main menu interface.
 */
void showMainMenu(void)
{
    setupMenuSys();

    LICENCES_NO = loadLicences();
    if (LICENCES_NO == -1)
    {
        printf("ERROR: could not load manifest.csv\n");
        return;
    }

    PROG_ENTRIES_NO = loadProgramEntries();
    if (PROG_ENTRIES_NO == -1)
    {
        printf("ERROR: could not load programs.csv\n");
        return;
    }

    int emacsInstalled = isProgramInstalled("emacs", 1);
    int gitInstalled = isProgramInstalled("git", 1);
    int tmuxInstalled = isProgramInstalled("tmux", 1);

    MenuItem rawMenu[] = {
        { 
            "",
            "Introduction",
            "",
            NULL,
            1,
            1
        },
        { 
            "intro",
            "Introduction to SHORK 486",
            "",
            printIntro,
            1
        },
        { 
            "pt1",
            "Public Test 1",
            "",
            printPT1,
            getIsPT1()
        },
        { 
            "started",
            "Getting started",
            "",
            printStarted,
            strncmp(OS_NAME, "SHORK 486", 9) == 0
        },
        { 
            "",
            "Software",
            "",
            NULL,
            1,
            1
        },
        {
            "cmdRef",
            "Command reference (WIP)",
            "",
            showCommandReference,
            PROG_ENTRIES_NO > 0
        },
        {
            "cmdList",
            "Commands & programs",
            "",
            printCommands,
            PROG_ENTRIES_NO > 0
        },
        { 
            "shorkutils",
            "SHORK Utilities",
            "",
            printSHORKUtilities,
            1
        },
        { 
            "shorktainment",
            "SHORK Entertainment",
            "",
            printSHORKEntertainment,
            isProgramInstalled("shorklocomotive", 1) || isProgramInstalled("shorksay", 1)
        },
        {
            "licences",
            "Licences",
            "",
            showLicencesMenu,
            LICENCES_NO > 0
        },
        { 
            "",
            "Guides",
            "",
            NULL,
            1,
            emacsInstalled || gitInstalled || tmuxInstalled
        },
        {
            "emacs",
            "Emacs (Mg) cheatsheet",
            "",
            printEmacsCheatsheet,
            emacsInstalled
        },
        {
            "git",
            "Supported Git commands",
            "",
            printGitCommands,
            gitInstalled
        },
        {
            "tmux",
            "tmux cheatsheet",
            "",
            printTmuxCheatsheet,
            tmuxInstalled
        }
    };
    int rawMenuSize = sizeof(rawMenu) / sizeof(rawMenu[0]);

    int running = 1;
    int cursorX = 1;
    int cursorY = 1;
    int cursorXPrev = 1;
    int cursorYPrev = 0;
    int fullRedraw = 1;

    // Filter menu to just what should actually be visible
    MenuItem menu[rawMenuSize];
    int menuSize = 0;
    for (int i = 0; i < rawMenuSize; i++)
        if (rawMenu[i].isVisible)
            menu[menuSize++] = rawMenu[i];

    while (running)
    {
        if (fullRedraw)
        {
            clearScreen();
            printHeader("SHORKHELP");
            printMenu(menu, menuSize, NULL, 1, TERM_SIZE.ws_col - 6, menuSize, &cursorX, &cursorY, &cursorXPrev, &cursorYPrev);
            printFooter("[jk] Navigate [Enter] Select [q] Quit");
        }
        else
        {
            if (COL_ENABLED)
                printf("\x1b[2;1H");
            else
                printf("\x1b[3;1H");
            printMenu(menu, menuSize, NULL, 1, TERM_SIZE.ws_col - 6, menuSize, &cursorX, &cursorY, &cursorXPrev, &cursorYPrev);
        }

        NavInput input = getNavInput();

        fullRedraw = 1;
        cursorYPrev = 0;
        switch (input)
        {
            case CURSOR_UP:
                cursorYPrev = cursorY;
                cursorY--;
                if (cursorY < 1) cursorY = menuSize;
                fullRedraw = 0;
                break;

            case CURSOR_DOWN:
                cursorYPrev = cursorY;
                cursorY++;
                if (cursorY > menuSize) cursorY = 1;
                fullRedraw = 0;
                break;

            case ENTER:
                clearScreen();
                menu[cursorY - 1].action();
                break;
        
            case QUIT:
                running = 0;
                break;

            case INVALID:
                fullRedraw = 0;
                break;
        }
    }

    clearScreen();
}

/**
 * Runs the licences menu interface.
 */
void showLicencesMenu(void)
{
    // Create a menu containing found program entries
    MenuItem menu[LICENCES_NO];
    for (int i = 0; i < LICENCES_NO; i++)
    {
        snprintf(menu[i].id, sizeof(menu[i].id), "%s", LICENCES[i].name);
        snprintf(menu[i].name, sizeof(menu[i].name), "%s", LICENCES[i].name);
        menu[i].action = NULL;
        menu[i].isVisible = 1;
        menu[i].isStatic = 0;
    }



    // Prepare for multi-column menu
    int colWidth = 22;
    int cols = TERM_SIZE.ws_col / (colWidth + 3);
    if (cols < 1) cols = 1;
    if (cols > LICENCES_NO) cols = LICENCES_NO;
    int rows = (LICENCES_NO + cols - 1) / cols;



    int running = 1;
    int cursorX = 1;
    int cursorY = 1;
    int cursorXPrev = 0;
    int cursorYPrev = 0;
    int maxY = 0;
    int fullRedraw = 1;

    while (running)
    {
        if (fullRedraw)
        {
            clearScreen();
            printHeader("Licences");
            printMenu(menu, LICENCES_NO, NULL, cols, colWidth, rows, &cursorX, &cursorY, &cursorXPrev, &cursorYPrev);
            printFooter("[hjkl] Navigate [Enter] Select [q] Back");
        }
        else
        {
            if (COL_ENABLED)
                printf("\x1b[2;1H");
            else
                printf("\x1b[3;1H");
            printMenu(menu, LICENCES_NO, NULL, cols, colWidth, rows, &cursorX, &cursorY, &cursorXPrev, &cursorYPrev);
        }

        NavInput input = getNavInput();

        fullRedraw = 1;
        cursorXPrev = cursorYPrev = 0;
        switch (input)
        {
            case CURSOR_LEFT:
                cursorXPrev = cursorX;
                cursorYPrev = cursorY;
                cursorX--;

                if (cursorX < 1) cursorX = cols;
                while ((maxY = rowsInCol(LICENCES_NO, rows, cursorX)) == 0)
                {
                    cursorX--;
                    if (cursorX < 1) cursorX = cols;
                }
                if (cursorY > maxY) cursorY = maxY;
                if (cursorY < 1) cursorY = maxY;

                fullRedraw = 0;
                break;

            case CURSOR_RIGHT:
                cursorXPrev = cursorX;
                cursorYPrev = cursorY;
                cursorX++;

                if (cursorX > cols) cursorX = 1;
                while ((maxY = rowsInCol(LICENCES_NO, rows, cursorX)) == 0)
                {
                    cursorX++;
                    if (cursorX > cols) cursorX = 1;
                }
                if (cursorY > maxY) cursorY = maxY;
                if (cursorY < 1) cursorY = maxY;

                fullRedraw = 0;
                break;

            case CURSOR_UP:
                cursorXPrev = cursorX;
                cursorYPrev = cursorY;
                cursorY--;

                if (cursorY < 1)
                    cursorY = rowsInCol(LICENCES_NO, rows, cursorX);

                fullRedraw = 0;
                break;

            case CURSOR_DOWN:
                cursorXPrev = cursorX;
                cursorYPrev = cursorY;
                cursorY++;

                if (cursorY > rowsInCol(LICENCES_NO, rows, cursorX))
                    cursorY = 1;
                    
                fullRedraw = 0;
                break;

            case ENTER:
                clearScreen();
                printLicence((cursorY - 1) + (cursorX - 1) * rows);
                break;
        
            case QUIT:
                running = 0;
                break;

            case INVALID:
                fullRedraw = 0;
                break;
        }
    }

    clearScreen();
}
