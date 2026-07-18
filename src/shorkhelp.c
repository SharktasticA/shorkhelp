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
        char *fields[10];
        int fieldCount = loadCSVLine(line, fields, 10);

        // Check if malformed line/parsing
        if (fieldCount < 10)
            continue;

        int isOptional = atoi(fields[2]);
        int progCheck = 0;
        // If the path field starts with '!', it means only proceed if the
        // executable in the path *is not* found. E.g., BusyBox and Vim provide
        // their own xxd - the BusyBox one should only be processed if Vim
        // isn't installed.
        if (fields[1][0] == '!')
        {
            if (!isProgramInstalled(fields[1] + 1, 1))
                progCheck = isProgramInstalled(fields[0], 1);
        }
        else
            progCheck = isProgramInstalled((fields[1] && fields[1][0] != '\0') ? fields[1] : fields[0], 1);

        if (!isOptional || (isOptional && progCheck))
        {
            // If no program name was given, try either package name or just
            // command name as a stand-in
            if (!fields[6] || fields[6][0] == '\0')
            {
                if (fields[4] && fields[4][0] != '\0')
                    fields[6] = fields[4];
                else
                    fields[6] = fields[0];
            }

            // Input line into entries
            PROG_ENTRIES[i].command = fields[0];
            PROG_ENTRIES[i].path = fields[1];
            PROG_ENTRIES[i].type = fields[3];
            PROG_ENTRIES[i].package = fields[4];
            PROG_ENTRIES[i].category = fields[5];
            PROG_ENTRIES[i].name = fields[6];
            PROG_ENTRIES[i].aliases = fields[7];

            if(strchr(fields[8], '"') != NULL)
            {
                // Remove doubled double quotes
                char *tmp = findReplace(fields[8], strlen(fields[8]), "\"\"", "\"");

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
            else PROG_ENTRIES[i].desc = fields[8];
            
            PROG_ENTRIES[i].licences = fields[9];

            i++;
        }
    }

    return i;
}



void printGuideDiscoveringHardware(void)
{
    const int strSize = 20480;
    char hardwareSize[strSize];
    int pos = 0;

    pos += snprintf(hardwareSize + pos, strSize - pos, "This guide will list and describe various ways you can find out more information about your system and its operating environment.\n\n");

    if (isProgramInstalled("shorkfetch", 1))
    {
        char *shorkfetch = captureProgramOutput("shorkfetch -cl=off", 1536);
        if (shorkfetch && shorkfetch[0] != '\0')
        {
            pos += snprintf(hardwareSize + pos, strSize - pos, "\033[%smOverall: shorkfetch\033[%sm\n", COL_FOR_HEADING, COL_FOR_WHITE);

            pos += snprintf(hardwareSize + pos, strSize - pos,
                "The easiest way to get an overall picture of your system is via a *fetch program. You will often see their output in screenshots and photos of Linux desktops online as they provide a convenient and pseudo-standardised way of showing the author's system configuration. \033[%smneofetch\033[%sm and \033[%smfastfetch\033[%sm are presently the most common for mainstream Linux distros - SHORK ships with its purpose-built \033[%smshorkfetch\033[%sm.\n\n",
                COL_FOR_CODE,  COL_FOR_WHITE,
                COL_FOR_CODE,  COL_FOR_WHITE,
                COL_FOR_SHORKUTIL,  COL_FOR_WHITE
            );

            pos += snprintf(hardwareSize + pos, strSize - pos, "Example output:\033[%sm\n%s", COL_FOR_CODE, shorkfetch);
        }
    }

    if (isProgramInstalled("free", 1))
    {
        char *free = captureProgramOutput("free -h", 512);
        if (free && free[0] != '\0')
        {
            pos += snprintf(hardwareSize + pos, strSize - pos, "\033[%smMemory: free\033[%sm\n", COL_FOR_HEADING, COL_FOR_WHITE);

            pos += snprintf(hardwareSize + pos, strSize - pos,
                "The \033[%smfree\033[%sm command will inform you how much total physical and swap memory is present, and how much of it is used or available. \033[%smfree -h\033[%sm is often used to make it print values in their most suitable unit (K/Ki, M/Mi, etc.) instead of bytes.\n\n",
                COL_FOR_CODE,  COL_FOR_WHITE,
                COL_FOR_CODE,  COL_FOR_WHITE
            );

            pos += snprintf(hardwareSize + pos, strSize - pos, "Example output:\033[%sm\n%s\n", COL_FOR_CODE, free);
        }
    }

    if (isProgramInstalled("df", 1))
    {
        char *df = captureProgramOutput("df -h", 2048);
        if (df && df[0] != '\0')
        {
            pos += snprintf(hardwareSize + pos, strSize - pos, "\033[%smStorage: df\033[%sm\n", COL_FOR_HEADING, COL_FOR_WHITE);

            pos += snprintf(hardwareSize + pos, strSize - pos,
                "The \033[%smdf\033[%sm (disk free) command will list each detected file system and inform you of each one's size, how much of it is used or available, and if/where it is mounted. \033[%smdf -h\033[%sm is often used to make it print values in their most suitable unit (K, M, etc.) instead of bytes.\n\n",
                COL_FOR_CODE,  COL_FOR_WHITE,
                COL_FOR_CODE,  COL_FOR_WHITE
            );

            pos += snprintf(hardwareSize + pos, strSize - pos, "Example output:\033[%sm\n%s\n", COL_FOR_CODE, df);
        }
    }

    if (isProgramInstalled("lsblk", 1))
    {
        char *lsblk = captureProgramOutput("lsblk", 1536);
        if (lsblk && lsblk[0] != '\0')
        {
            pos += snprintf(hardwareSize + pos, strSize - pos, "\033[%smStorage: lsblk\033[%sm\n", COL_FOR_HEADING, COL_FOR_WHITE);

            pos += snprintf(hardwareSize + pos, strSize - pos,
                "The \033[%smlsblk\033[%sm (list block device) command will list each detected storage device and its partitions, or loop device, and inform you of each one's major and minor numbers, its size, its device type, and its mountpoint.\n\n",
                COL_FOR_CODE,  COL_FOR_WHITE
            );

            pos += snprintf(hardwareSize + pos, strSize - pos, "Example output:\033[%sm\n%s\n", COL_FOR_CODE, lsblk);
        }
    }

    if (isProgramInstalled("fdisk", 1))
    {
        char *fdisk = captureProgramOutput("fdisk -l", 1024);
        if (fdisk && fdisk[0] != '\0')
        {
            limitLines(fdisk, 7);

            pos += snprintf(hardwareSize + pos, strSize - pos, "\033[%smStorage: fdisk\033[%sm\n", COL_FOR_HEADING, COL_FOR_WHITE);

            pos += snprintf(hardwareSize + pos, strSize - pos,
                "The \033[%smfdisk\033[%sm command is a disk partitioning utility that can be used to describe each partition found via \033[%smfdisk -l\033[%sm. For each partition, it will inform you of its host device, if it is bootable, its start and end CHS and LBA geometry, its sector count, its size and its type.\n\n",
                COL_FOR_CODE,  COL_FOR_WHITE,
                COL_FOR_CODE,  COL_FOR_WHITE
            );

            pos += snprintf(hardwareSize + pos, strSize - pos, "Example output:\033[%sm\n%s\n", COL_FOR_CODE, fdisk);
        }
    }

    if (isProgramInstalled("lscpu", 1))
    {
        char *lscpu = captureProgramOutput("lscpu", 1024);
        if (lscpu && lscpu[0] != '\0')
        {
            limitLines(lscpu, 10);

            pos += snprintf(hardwareSize + pos, strSize - pos, "\033[%smProcessor: lscpu\033[%sm\n", COL_FOR_HEADING, COL_FOR_WHITE);

            pos += snprintf(hardwareSize + pos, strSize - pos,
                "The \033[%smlscpu\033[%sm (list CPU information) command lists information about your processor, including its architecture, its vendor and model names, how many cores or threads it has, what CPU flags it and the Linux kernel reports and what vulnerabilities affect it. It parses data from \033[%sm/proc/cpuinfo\033[%sm and \033[%smsysfs\033[%sm. Its output can be large - you can pipe it to \033[%smless\033[%sm to make it more manageable.\n\n",
                COL_FOR_CODE,  COL_FOR_WHITE,
                COL_FOR_CODE,  COL_FOR_WHITE,
                COL_FOR_CODE,  COL_FOR_WHITE,
                COL_FOR_CODE,  COL_FOR_WHITE
            );

            pos += snprintf(hardwareSize + pos, strSize - pos, "Example output:\033[%sm\n%s\n", COL_FOR_CODE, lscpu);
        }
    }

    char *cpuinfo = captureProgramOutput("cat /proc/cpuinfo", 1024);
    if (cpuinfo && cpuinfo[0] != '\0')
    {
        limitLines(cpuinfo, 10);

        pos += snprintf(hardwareSize + pos, strSize - pos, "\033[%smProcessor: cpuinfo\033[%sm\n", COL_FOR_HEADING, COL_FOR_WHITE);

        pos += snprintf(hardwareSize + pos, strSize - pos,
            "\033[%sm/proc/cpuinfo\033[%sm is a file produced by the Linux kernel as part of \033[%smprocfs\033[%sm that reports information about your processor. You can read it with \033[%smcat\033[%sm. It includes its architecture, its vendor and model names, how many cores or threads it has, what CPU flags it and the Linux kernel reports, and what vulnerabilities affect it. It is often used as a source of data by *fetch commands and \033[%smlscpu\033[%sm (if installed), being less human-readable than those but more often present. Its output can be large - you can pipe it to \033[%smless\033[%sm to make it more manageable.\n\n",
            COL_FOR_CODE,  COL_FOR_WHITE,
            COL_FOR_CODE,  COL_FOR_WHITE,
            COL_FOR_CODE,  COL_FOR_WHITE,
            COL_FOR_CODE,  COL_FOR_WHITE,
            COL_FOR_CODE,  COL_FOR_WHITE
        );

        pos += snprintf(hardwareSize + pos, strSize - pos, "Example output:\033[%sm\n%s\n", COL_FOR_CODE, cpuinfo);
    }

    if (isProgramInstalled("lspci", 1))
    {
        char *lspci = captureProgramOutput("lspci", 512);
        if (lspci && lspci[0] != '\0')
        {
            limitLines(lspci, 5);

            pos += snprintf(hardwareSize + pos, strSize - pos, "\033[%smPeripherals: lspci\033[%sm\n", COL_FOR_HEADING, COL_FOR_WHITE);

            pos += snprintf(hardwareSize + pos, strSize - pos,
                "The \033[%smlspci\033[%sm (list PCI devices) command lists all detected PCI and PCI Express buses and devices connected to the computer. For each device, it informs the user its PCI address (bus:device:function), its class code (e.g. 0300 for VGA-compatible graphics) and its vendor:device IDs. BusyBox's implementation does not name the device or its type, but the class codes, and the vendor and device IDs, can be used when searching \033[%smadmin.pci-ids.ucw.cz/read/PD/\033[%sm and \033[%smadmin.pci-ids.ucw.cz/read/PC/\033[%sm (respectively) to identify the device.\n\n",
                COL_FOR_CODE,  COL_FOR_WHITE,
                COL_FOR_CODE,  COL_FOR_WHITE,
                COL_FOR_CODE,  COL_FOR_WHITE
            );

            pos += snprintf(hardwareSize + pos, strSize - pos, "Example output:\033[%sm\n%s", COL_FOR_CODE, lspci);
        }
    }

    /*if (isProgramInstalled("lsusb", 1))
    {
        // TODO
    }*/

    int lines = formatNewLines(hardwareSize, TERM_SIZE.ws_col, NULL, 1);
    printTextScreen("Discovering your hardware", hardwareSize, lines, 1);
}

void printGuideEmacsCheatsheet(void)
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

void printGuideGitCommands(void)
{
    char gitStr[500];
    snprintf(gitStr, 500, "add, blame, branch, checkout, cherry-pick, clean, clone, commit, config, diff, fetch, init, log, merge, mv, pull, push, rebase, reflog, remote, reset, restore, rm, show, stash, status, switch, tag\n");

    int lines = formatNewLines(gitStr, TERM_SIZE.ws_col, NULL, 1);
    printTextScreen("Supported Git commands", gitStr, lines, 1);
}

void printGuideTmuxCheatsheet(void)
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



void printIntro(void)
{
    const int strSize = 4192;
    char introStr[strSize];
    int pos = 0;

    pos += snprintf(introStr + pos, strSize - pos, "SHORK 486 is a 32-bit Linux distribution for 486 and Pentium (P5) PCs! It focuses on being as lean and small as possible, whilst still providing a modern kernel, robust command set, custom utilities, and hand-picked modern software.\n\n");

    pos += snprintf(introStr + pos, strSize - pos, "\033[%smOrigin & inspirations\033[%sm\nIn December 2025, YouTuber Action Retro posted a video on FLOPPINUX, something that turned out to be a very accessible means for me to learn how to make a working Linux system. It foremost inspired me to chase my dream of building a viable modern Linux system for my old IBM ThinkPads. SHORK Mini (as it was originally known) began as an automated build script based on FLOPPINUX\'s build instructions, but adapted for producing fixed disk images instead of diskette images. After that, more Linux kernel and BusyBox features were enabled, and other software was compiled to help fulfil that dream. Other inspirations from similar efforts include Gray386linux and Ocawesome101\'s blog post on running Linux on a 486SX. Aspects of Alpine Linux and Tiny Core are also kept in mind.\n\n", COL_FOR_HEADING, COL_FOR_WHITE);

    pos += snprintf(introStr + pos, strSize - pos, "\033[%smArchitecture\033[%sm\nSHORK 486 is not GNU/Linux as you may be accustomed to. Its init system and most core utilities are provided by BusyBox, a single-binary application well known for embedded usage. As needed, some util-linux and individual utilities are used to fill holes in BusyBox\'s suite. The system is compiled with musl instead of glibc, producing smaller binaries that also use fewer resources. The closest well-known Linux distribution to SHORK 486 is Alpine Linux.\n\n", COL_FOR_HEADING, COL_FOR_WHITE);

    pos += snprintf(introStr + pos, strSize - pos, "\033[%smGoals\033[%sm\nBesides being something fun to try on old PCs, SHORK 486 was founded on the belief that old PCs can still be useful to the right people, retrocomputing and gaming usage aside. SHORK 486 can be useful for lightweight desktop usage, SSH terminal usage, distraction-free typewriting (\"writerdecks\"), embedded applications, demonstrative use in academia/education, and as a technical demonstration of what old PCs and modern software targeted at them can actually still do! If it can save just one more PC from the landfills, it has done its job!\n\n", COL_FOR_HEADING, COL_FOR_WHITE);

    pos += snprintf(introStr + pos, strSize - pos, "\033[%smWhat's special\033[%sm\nSHORK 486 is a modern and maintained Linux distribution that can run on a processor architecture from 1989. Depending on configuration, it only requires between 8 and 24MiB system memory whilst still packing a lot of functionality for its size. Due to various factors, making such a distribution is increasingly difficult in the 2020s. System requirements are ever-increasing, with even the otherwise excellent Micro Core and Tiny Core requiring at least 26-46MB RAM, putting them out of range of many early 486 systems. As of Linux kernel 7.1 and beyond, support for 486 processors and various ISA and PCMCIA networking hardware has been dropped, and 32-bit x86 support in general is currently dropped by most mainstream distributions. Given this situation, SHORK 486 will try to fill this niche of a ready-to-go Linux distribution for such PCs by sticking with a minimal-where-possible philosophy, customisability and restoring support for older hardware with newer Linux kernels!\n\n", COL_FOR_HEADING, COL_FOR_WHITE);

    pos += snprintf(introStr + pos, strSize - pos, "\033[%smLicences\033[%sm\nSHORK 486 is a free and open-source operating system. Its core is made up of GPLv3 (SHORK, SHORK Utilities, SHORK Entertainment), GPLv2 (Linux kernel, BusyBox, SYSLINUX), and MIT (SHORKMINES) components. SHORK 486 can also contain bundled software licensed under various permissive, copyleft, and even public-domain-equivalent licences. You can look at \033[%smshorkhelp\033[%sm's \"Licences\" portal to see their licences.", COL_FOR_HEADING, COL_FOR_WHITE, COL_FOR_SHORKUTIL, COL_FOR_WHITE);

    int lines = formatNewLines(introStr, TERM_SIZE.ws_col, NULL, 0);
    printTextScreen("Introduction to SHORK 486", introStr, lines, 1);
}

void printIntroPT1(void)
{
    const int strSize = 3072;
    char pt1Str[strSize];
    int pos = 0;

    pos += snprintf(pt1Str + pos, strSize - pos, "You are running a pre-release build of SHORK 486 - Public Test 1 (PT1). This is the first published release of SHORK 486, intended for testing purposes only and to gauge wider opinion to help the project's progression.\n\n");

    pos += snprintf(pt1Str + pos, strSize - pos, "\033[%smPurpose\033[%sm\nSHORK 486's development has progressed far enough that it has achieved the author's initial objectives, and is now appropriate for the first round of public testing to help scope out what to tackle next. PT1 also coincides with Linux kernel 7.1's debut, the first version to remove 486 and other relevant vintage hardware support, which this project subsequently patches back in. It would be ideal to validate that these patches are working correctly.\n\n", COL_FOR_HEADING, COL_FOR_WHITE);

    pos += snprintf(pt1Str + pos, strSize - pos, "\033[%smLicence & disclaimers\033[%sm\nSHORK 486 is a non-commercial, free and open-source operating system licenced under GPLv3 that is provided \"as is\" without warranty of any kind. In no event shall the contributors be liable for any damages arising from use of this software. Additionally, SHORK 486 is a work in progress, and this is a pre-release build. It should not be used for serious, production, or mission-critical work.\n\n", COL_FOR_HEADING, COL_FOR_WHITE);

    pos += snprintf(pt1Str + pos, strSize - pos, "\033[%smSuggestions & feedback\033[%sm\nIdeas for what to add and change to SHORK 486 are welcome! These can be suggestions for new bundled programs, new or improved features, and advice on how the system is configured and built. Frank and critical feedback on anything regarding the project is also welcomed, though it is expected that it is respectful and understanding. However, please understand that otherwise well-intentioned and interesting suggestions may be considered beyond the scope of this project, as the aim is to keep the system as minimal as possible, but they will be remembered for future sister projects. Suggestions and feedback can be given via the SHORK 486 GitHub repository's \"Discussions\" tab or via social media channels found on \033[%smlinks.sharktastica.co.uk\033[%sm.\n\n", COL_FOR_HEADING, COL_FOR_WHITE, COL_FOR_CODE, COL_FOR_WHITE);

    pos += snprintf(pt1Str + pos, strSize - pos, "\033[%smBug & error reporting\033[%sm\nSHORK 486 does not collect any telemetry nor report anything to anyone, instead relying on you, the tester, to report and provide evidence of any issues that may arise when using it. If you encounter any, please report them! It is a good measure to include a robust description of your hardware and what you were trying to do, whether you compiled it yourself or used published install media, a screenshot/photo of \033[%smshorkfetch\033[%sm's output, \033[%sm/proc/cpuinfo\033[%sm's contents, and \033[%smdmesg\033[%sm output. Reports can be given via the SHORK 486 GitHub repository's \"Issues\" tab.", COL_FOR_HEADING, COL_FOR_WHITE, COL_FOR_CODE, COL_FOR_WHITE, COL_FOR_CODE, COL_FOR_WHITE, COL_FOR_CODE, COL_FOR_WHITE);

    int lines = formatNewLines(pt1Str, TERM_SIZE.ws_col, NULL, 0);
    printTextScreen("Public Test 1", pt1Str, lines, 1);
}

void printIntroStarted(void)
{
    const int strSize = 4096;
    char startedStr[strSize];
    int pos = 0;
    int item = 1;



    if (isProgramInstalled("shorkfetch", 1))
    {
        pos += snprintf(startedStr + pos, strSize - pos,
            "\033[%sm%d. \033[%smKnowing your system\033[%sm\n", 
            COL_FOR_OL, item++,
            COL_FOR_HEADING, COL_FOR_WHITE
        );

        pos += snprintf(startedStr + pos, strSize - pos,
            "You can run \033[%smshorkfetch\033[%sm to see a quick overview of your system and environment. Commands like \033[%smlscpu\033[%sm (processor), \033[%smfree\033[%sm (memory) and \033[%smlsblk\033[%sm (drives) are available for specific information, and are explained in \033[%smshorkhelp\033[%sm's \"discovering your hardware\" guide. \033[%smshorkhelp\033[%sm also provides a command reference portal and \"commands & programs\" list for you to learn what software is available and what you can do with your system.\n\n",
            COL_FOR_SHORKUTIL,  COL_FOR_WHITE,
            COL_FOR_CODE,  COL_FOR_WHITE,
            COL_FOR_CODE,  COL_FOR_WHITE,
            COL_FOR_CODE,  COL_FOR_WHITE,
            COL_FOR_SHORKUTIL,  COL_FOR_WHITE,
            COL_FOR_SHORKUTIL,  COL_FOR_WHITE
        );
    }



    if (isProgramInstalled("shorkset", 1))
    {
        pos += snprintf(startedStr + pos, strSize - pos,
            "\033[%sm%d. \033[%smSetting your preferences\033[%sm\n", 
            COL_FOR_OL, item++,
            COL_FOR_HEADING, COL_FOR_WHITE
        );

        pos += snprintf(startedStr + pos, strSize - pos,
            "\033[%smshorkset\033[%sm is your port of call if you want to customise your experience. The exact options depends on your hardware and bundled relevant software, but it can allow you to select a VGA or VESA-style display resolution, keyboard layout (keymap), PSF-format console font, console font colour and sound volume.\n\n",
            COL_FOR_SHORKUTIL,  COL_FOR_WHITE
        );
    }



    pos += snprintf(startedStr + pos, strSize - pos,
        "\033[%sm%d. \033[%smChanging your computer's name\033[%sm\n", 
        COL_FOR_OL, item++,
        COL_FOR_HEADING, COL_FOR_WHITE
    );

    pos += snprintf(startedStr + pos, strSize - pos,
        "Your computer's name is stored in \033[%sm/etc/hostname\033[%sm and you can change it by editing that file. A reboot will be required, or run \033[%smhostname \"$(cat /etc/hostname)\"\033[%sm to apply the change immediately.\n\n",
        COL_FOR_CODE,  COL_FOR_WHITE,
        COL_FOR_CODE,  COL_FOR_WHITE
    );



    if (isProgramInstalled("ping", 1))
    {
        pos += snprintf(startedStr + pos, strSize - pos,
            "\033[%sm%d. \033[%smTesting your network connection\033[%sm\n", 
            COL_FOR_OL, item++,
            COL_FOR_HEADING, COL_FOR_WHITE
        );

        pos += snprintf(startedStr + pos, strSize - pos,
            "If you intend to use the internet, you can use \033[%smping\033[%sm - for example, \033[%smping sharktastica.co.uk\033[%sm - to quickly check if you can reach a website. If you cannot, please note that at present, your ethernet cable must be plugged into your computer before starting SHORK. A network manager is in development to address this in the future.\n\n",
            COL_FOR_CODE,  COL_FOR_WHITE,
            COL_FOR_CODE,  COL_FOR_WHITE
        );
    }



    if (isProgramInstalled("chvt", 1))
    {
        pos += snprintf(startedStr + pos, strSize - pos,
            "\033[%sm%d. \033[%smHow to multitask\033[%sm\n", 
            COL_FOR_OL, item++,
            COL_FOR_HEADING, COL_FOR_WHITE
        );

        pos += snprintf(startedStr + pos, strSize - pos,
            "At a minimum, SHORK provides three virtual consoles (ttyX) you can access with Ctrl+Alt+[F1/F2/F3] or \033[%smchvt [1/2/3]\033[%sm. They allow you to do something else without closing the current program or to bypass a crashed program.\n\n",
            COL_FOR_CODE,  COL_FOR_WHITE
        );

        if (isProgramInstalled("tmux", 1))
        {
            pos += snprintf(startedStr + pos, strSize - pos,
                "Additionally, \033[%smtmux\033[%sm is available, which allows you to split each console into multiple \"windows\" and multiple panes inside them. Each one can launch a program. It requires memorising several keyboard commands, but \033[%smshorkhelp\033[%sm's tmux cheatsheet is available to help get you started.\n\n",
                COL_FOR_CODE,  COL_FOR_WHITE,
                COL_FOR_SHORKUTIL,  COL_FOR_WHITE
            );
        }
    }



    if (isProgramInstalled("shorkmatrix", 1))
    {
        pos += snprintf(startedStr + pos, strSize - pos,
            "\033[%sm%d. \033[%smSaving your screen\033[%sm\n", 
            COL_FOR_OL, item++,
            COL_FOR_HEADING, COL_FOR_WHITE
        );

        pos += snprintf(startedStr + pos, strSize - pos,
            "If you have a CRT, plasma or OLED-based monitor, and you are going to step away from your computer for a while without turning it off, you can run \033[%smshorkmatrix\033[%sm before you go to serve as your screensaver to help prevent burn-in.\n\n",
            COL_FOR_SHORKUTIL,  COL_FOR_WHITE
        );
    }



    if (isProgramInstalled("shorkoff", 1))
    {
        pos += snprintf(startedStr + pos, strSize - pos,
            "\033[%sm%d. \033[%smShutting down for the day\033[%sm\n", 
            COL_FOR_OL, item++,
            COL_FOR_HEADING, COL_FOR_WHITE
        );

        pos += snprintf(startedStr + pos, strSize - pos,
            "When you are finished using your computer, it is recommended to run \033[%smshorkoff\033[%sm before you press its power button. It safely halts your system and syncs any write cache to your drives, helping prevent data loss if your drives are perhaps too slow to do it automatically in time.\n\n",
            COL_FOR_SHORKUTIL,  COL_FOR_WHITE
        );
    }



    int lines = formatNewLines(startedStr, TERM_SIZE.ws_col, NULL, 1);
    printTextScreen("Getting started", startedStr, lines, 1);
}



void printSoftwareCommands(void)
{
    char genStr[MAX_CMD_STR] = "\0";
    char arcStr[MAX_CMD_STR] = "\0";
    char txtStr[MAX_CMD_STR] = "\0";
    char devStr[MAX_CMD_STR] = "\0";
    char chkStr[MAX_CMD_STR] = "\0";
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
        else if (strcmp(cat, "txt") == 0) targetStr = txtStr;
        else if (strcmp(cat, "arc") == 0) targetStr = arcStr;
        else if (strcmp(cat, "dev") == 0) targetStr = devStr;
        else if (strcmp(cat, "chk") == 0) targetStr = chkStr;
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
    pos += snprintf(combinedStr + pos, combinedSize - pos, "\033[%smArchival\033[%sm\n%s\n\n", COL_FOR_HEADING, COL_FOR_WHITE, arcStr);
    pos += snprintf(combinedStr + pos, combinedSize - pos, "\033[%smText processing\033[%sm\n%s\n\n", COL_FOR_HEADING, COL_FOR_WHITE, txtStr);
    pos += snprintf(combinedStr + pos, combinedSize - pos, "\033[%smEditors & development\033[%sm\n%s\n\n", COL_FOR_HEADING, COL_FOR_WHITE, devStr);
    pos += snprintf(combinedStr + pos, combinedSize - pos, "\033[%smChecksums & hashing\033[%sm\n%s\n\n", COL_FOR_HEADING, COL_FOR_WHITE, chkStr);
    if (netEnabled) pos += snprintf(combinedStr + pos, combinedSize - pos, "\033[%smNetworking & remote access\033[%sm\n%s\n\n", COL_FOR_HEADING, COL_FOR_WHITE, netStr);
    pos += snprintf(combinedStr + pos, combinedSize - pos, "\033[%smSystem & processes\033[%sm\n%s\n", COL_FOR_HEADING, COL_FOR_WHITE, sysStr);
    if (usrEnabled) pos += snprintf(combinedStr + pos, combinedSize - pos, "\n\033[%smUser management\033[%sm\n%s\n", COL_FOR_HEADING, COL_FOR_WHITE, usrStr);
    if (funEnabled) pos += snprintf(combinedStr + pos, combinedSize - pos, "\n\033[%smEntertainment\033[%sm\n%s\n", COL_FOR_HEADING, COL_FOR_WHITE, funStr);
    if (ustEnabled) pos += snprintf(combinedStr + pos, combinedSize - pos, "\n\033[%smUnsorted\033[%sm\n%s\n", COL_FOR_HEADING, COL_FOR_WHITE, ustStr);

    int lines = formatNewLines(combinedStr, TERM_SIZE.ws_col, NULL, 1);
    printTextScreen("Commands & programs list", combinedStr, lines, 1);
}

void printSoftwareLicence(int i)
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

void printSoftwareProgOverview(int i)
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

    const char *type = PROG_ENTRIES[i].type;
    if (strcmp(type, "busybox") == 0)
        type = "BusyBox";
    else if (strcmp(type, "shorkutil") == 0)
        type = "SHORK Utilities";
    else if (strcmp(type, "shorktainment") == 0)
        type = "SHORK Entertainment";
    else if (strcmp(type, "bundled") == 0)
        type = "bundled software";
    pos += snprintf(overviewStr + pos, strSize - pos, "\033[%smSource:\033[%sm   %s\n", COL_FOR_OL, COL_RESET, type);

    if (PROG_ENTRIES[i].package && PROG_ENTRIES[i].package[0] != '\0')
        pos += snprintf(overviewStr + pos, strSize - pos, "\033[%smPackage:\033[%sm  %s\n", COL_FOR_OL, COL_RESET, PROG_ENTRIES[i].package);

    const char *category = PROG_ENTRIES[i].category;
    if (strcmp(category, "gen") == 0)
        category = "general";
    else if (strcmp(category, "arc") == 0)
        category = "archival";
    else if (strcmp(category, "txt") == 0)
        category = "text processing";
    else if (strcmp(category, "dev") == 0)
        category = "editors & development";
    else if (strcmp(category, "chk") == 0)
        category = "checksums & hashing";
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

    if (PROG_ENTRIES[i].licences && PROG_ENTRIES[i].licences[0] != '\0')
        pos += snprintf(overviewStr + pos, strSize - pos, "\033[%smLicences:\033[%sm %s\n", COL_FOR_OL, COL_RESET, PROG_ENTRIES[i].licences);

    int lines = formatNewLines(overviewStr, TERM_SIZE.ws_col, NULL, 1);
    printTextScreen(PROG_ENTRIES[i].command, overviewStr, lines, 1);
}

void printSoftwareSHORKTAINMENT(void)
{
    const int strSize = 2000;
    char shorktainmentStr[strSize];
    int pos = 0;

    if (isProgramInstalled("sl", 1))
        pos += snprintf(shorktainmentStr + pos, strSize - pos, "\033[%smshorklocomotive\033[%sm\nA shark-themed take on the \033[%smsl\033[%sm command that kindly pokes fun at making typos when trying to type \033[%smls\033[%sm. A cute SHORK will swim across the screen.\n\n", COL_FOR_SHORKUTIL, COL_FOR_WHITE, COL_FOR_CODE, COL_FOR_WHITE, COL_FOR_CODE, COL_FOR_WHITE);

    if (isProgramInstalled("shorkmatrix", 1))
        pos += snprintf(shorktainmentStr + pos, strSize - pos, "\033[%smshorkmatrix\033[%sm\nA minimalist, blue-themed take on the \033[%smcmatrix\033[%sm \"digital rain\" vertical scrolling text screensaver. Inspired by the 1999 film \"The Matrix\", the \"droplets\" are lines of blue characters that fall from the top of the terminal to the bottom, and are occasionally broken up.\n\n", COL_FOR_SHORKUTIL, COL_FOR_WHITE, COL_FOR_CODE, COL_FOR_WHITE);

    if (isProgramInstalled("shorkmines", 1))
        pos += snprintf(shorktainmentStr + pos, strSize - pos, "\033[%smshorkmines\033[%sm\nAn ncurses-based minesweeper game based on \033[%smterminal-mines\033[%sm. You can customise the game's difficulty by entering a width, height, and mine density of your choosing.\n\n", COL_FOR_SHORKUTIL, COL_FOR_WHITE, COL_FOR_CODE, COL_FOR_WHITE);

    if (isProgramInstalled("shorksay", 1))
        pos += snprintf(shorktainmentStr + pos, strSize - pos, "\033[%smshorksay\033[%sm\nA shark-themed take on the \033[%smcowsay\033[%sm command that outputs a cute SHORK with a speech bubble containing a message of your choice. The message can be manually supplied or can be provided from another program's standard output (ie, using a pipe).\n\n", COL_FOR_SHORKUTIL, COL_FOR_WHITE, COL_FOR_CODE, COL_FOR_WHITE);

    int lines = formatNewLines(shorktainmentStr, TERM_SIZE.ws_col, NULL, 1);
    printTextScreen("SHORK Entertainment", shorktainmentStr, lines, 1);
}

void printSoftwareSHORKUTILS(void)
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



void printOtherReport(void)
{
    char msgBody[6144];

    snprintf(msgBody, 6144, "The after-build report is a log generated after SHORK 486 finished compiling. It is used to confirm whether the build completed as intended, and it should provide all the information needed to reproduce the build.\n\n");

    FILE *stream = fopen(BUILD_REPORT_PATH, "r");
    if (stream)
    {
        size_t len = strlen(msgBody);
        size_t bytesRead = fread(msgBody + len, 1, sizeof(msgBody) - 1, stream);
        fclose(stream);
        msgBody[bytesRead + len] = '\0';
    }
    else
    {
        size_t extMsgLen = 48 + strlen(BUILD_REPORT_PATH);
        EXIT_MSG = malloc(extMsgLen);
        snprintf(EXIT_MSG, extMsgLen, "ERROR: could not load %s", BUILD_REPORT_PATH);
        exit(1);
    }

    int lines = formatNewLines(msgBody, TERM_SIZE.ws_col, NULL, 0);
    printTextScreen("After-build report", msgBody, lines, 1);
}

void printOtherSupport(void)
{
    const int strSize = 1024;
    char supportStr[strSize];
    snprintf(supportStr, strSize,
"\033[%smSource code\033[%sm\n\
 * SHORK 486 GitHub repository (\033[%smgithub.com/SharktasticA/SHORK-486\033[%sm)\n\n\
\033[%smSuggestions & feedback\033[%sm\n\
 * SHORK 486 GitHub repository's \"Discussions\" tab\n\
 * Contact author on social media (\033[%smlinks.sharktastica.co.uk\033[%sm)\n\
 * Contact author via email (\033[%smsharksibmstuff@gmail.com\033[%sm)\n\n\
\033[%smBug & error reporting\033[%sm\n\
 * SHORK 486 GitHub repository's \"Issues\" tab",
        COL_FOR_HEADING, COL_FOR_WHITE,
        COL_FOR_CODE, COL_FOR_WHITE,
        COL_FOR_HEADING, COL_FOR_WHITE,
        COL_FOR_CODE, COL_FOR_WHITE,
        COL_FOR_CODE, COL_FOR_WHITE,
        COL_FOR_HEADING, COL_FOR_WHITE
    );

    int lines = formatNewLines(supportStr, TERM_SIZE.ws_col, "   ", 0);
    printTextScreen("Getting support or involved", supportStr, lines, 1);
}



/**
 * Runs the command reference interface.
 */
void showCommandRefMenu(void)
{
    // Create a menu containing found program entries
    MenuItem menu[PROG_ENTRIES_NO];
    for (int i = 0; i < PROG_ENTRIES_NO; i++)
    {
        snprintf(menu[i].id, sizeof(menu[i].id), "%s", PROG_ENTRIES[i].command);
        snprintf(menu[i].name, sizeof(menu[i].name), "%s", PROG_ENTRIES[i].command);
        menu[i].payload = NULL;
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
                printSoftwareProgOverview((cursorY - 1) + (cursorX - 1) * rows);
                break;
        
            case QUIT:
                running = 0;
                break;

            case INVALID:
                fullRedraw = 0;
                break;
        }
    }

    freeMenu(menu, PROG_ENTRIES_NO);
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

    char commands[100] = "--commands       Displays commands & programs list\n";
    formatNewLines(commands, TERM_SIZE.ws_col, "                 ", 0);
    printf("%s", commands);

    if (isProgramInstalled("emacs", 1) || isProgramInstalled("mg", 1))
    {
        char emacs[100] = "--emacs          Displays Emacs (Mg) cheatsheet\n";
        formatNewLines(emacs, TERM_SIZE.ws_col, "                 ", 0);
        printf("%s", emacs);
    }

    char hardware[80] = "--hardware       Displays discovering your hardware guide\n";
    formatNewLines(hardware, TERM_SIZE.ws_col, "                 ", 0);
    printf("%s", hardware);

    char help[100] = "-h, --help       Displays help information and exits\n";
    formatNewLines(help, TERM_SIZE.ws_col, "                 ", 0);
    printf("%s", help);

    if (isProgramInstalled("git", 1))
    {
        char git[100] = "--git            Displays supported Git commands\n";
        formatNewLines(git, TERM_SIZE.ws_col, "                 ", 0);
        printf("%s", git);
    }

    char intro[100] = "--intro          Displays introduction to SHORK 486\n";
    formatNewLines(intro, TERM_SIZE.ws_col, "                 ", 0);
    printf("%s", intro);

    char licences[100] = "--licences       Displays licences menu\n";
    formatNewLines(licences, TERM_SIZE.ws_col, "                 ", 0);
    printf("%s", licences);

    char nc[80] = "-nc, --no-col    Disables all coloured output\n";
    formatNewLines(nc, TERM_SIZE.ws_col, "                 ", 0);
    printf("%s", nc);

    if (getIsPT1())
    {
        char pt1[80] = "--pt1            Displays Public Test 1 information\n";
        formatNewLines(pt1, TERM_SIZE.ws_col, "                 ", 0);
        printf("%s", pt1);
    }

    if (access(BUILD_REPORT_PATH, F_OK) == 0)
    {
        char report[60] = "--report         Displays after-build report\n";
        formatNewLines(report, TERM_SIZE.ws_col, "                 ", 0);
        printf("%s", report);
    }

    if (isProgramInstalled("sl", 1) || isProgramInstalled("shorkmatrix", 1) || isProgramInstalled("shorkmines", 1) || isProgramInstalled("shorksay", 1))
    {
        char shorktainment[100] = "--shorktainment  Displays SHORK Entertainment list\n";
        formatNewLines(shorktainment, TERM_SIZE.ws_col, "                 ", 0);
        printf("%s", shorktainment);
    }

    char shorkutils[100] = "--shorkutils     Displays SHORK Utilities list\n";
    formatNewLines(shorkutils, TERM_SIZE.ws_col, "                 ", 0);
    printf("%s", shorkutils);

    if (strncmp(OS_NAME, "SHORK 486", 9) == 0)
    {
        char started[100] = "--started        Displays getting started guide\n";
        formatNewLines(started, TERM_SIZE.ws_col, "                 ", 0);
        printf("%s", started);
    }

    char support[100] = "--support        Displays getting support & getting involved information\n";
    formatNewLines(support, TERM_SIZE.ws_col, "                 ", 0);
    printf("%s", support);

    if (isProgramInstalled("tmux", 1))
    {
        char tmux[100] = "--tmux           Displays tmux cheatsheet\n";
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
        EXIT_MSG = strdup("ERROR: could not load manifest.csv");
        exit(1);
    }

    PROG_ENTRIES_NO = loadProgramEntries();
    if (PROG_ENTRIES_NO == -1)
    {
        EXIT_MSG = strdup("ERROR: could not load programs.csv");
        exit(1);
    }

    int emacsInstalled = isProgramInstalled("emacs", 1);
    int gitInstalled = isProgramInstalled("git", 1);
    int tmuxInstalled = isProgramInstalled("tmux", 1);

    MenuItem rawMenu[] = {
        { 
            "",
            "Introduction",
            NULL,
            NULL,
            1,
            1
        },
        { 
            "intro",
            "Introduction to SHORK 486",
            NULL,
            printIntro,
            1
        },
        { 
            "pt1",
            "Public Test 1",
            NULL,
            printIntroPT1,
            getIsPT1()
        },
        { 
            "started",
            "Getting started",
            NULL,
            printIntroStarted,
            strncmp(OS_NAME, "SHORK 486", 9) == 0
        },
        { 
            "",
            "Software",
            NULL,
            NULL,
            1,
            1
        },
        {
            "cmdRef",
            "Command reference (WIP)",
            NULL,
            showCommandRefMenu,
            PROG_ENTRIES_NO > 0
        },
        {
            "cmdList",
            "Commands & programs",
            NULL,
            printSoftwareCommands,
            PROG_ENTRIES_NO > 0
        },
        { 
            "shorkutils",
            "SHORK Utilities",
            NULL,
            printSoftwareSHORKUTILS,
            1
        },
        { 
            "shorktainment",
            "SHORK Entertainment",
            NULL,
            printSoftwareSHORKTAINMENT,
            isProgramInstalled("shorklocomotive", 1) || isProgramInstalled("shorksay", 1)
        },
        {
            "licences",
            "Licences",
            NULL,
            showLicencesMenu,
            LICENCES_NO > 0
        },
        { 
            "",
            "Guides",
            NULL,
            NULL,
            1,
            1
        },
        {
            "hardware",
            "Discovering your hardware",
            NULL,
            printGuideDiscoveringHardware,
            1
        },
        {
            "emacs",
            "Emacs (Mg) cheatsheet",
            NULL,
            printGuideEmacsCheatsheet,
            emacsInstalled
        },
        {
            "git",
            "Supported Git commands",
            NULL,
            printGuideGitCommands,
            gitInstalled
        },
        {
            "tmux",
            "tmux cheatsheet",
            NULL,
            printGuideTmuxCheatsheet,
            tmuxInstalled
        },
        { 
            "",
            "Other",
            NULL,
            NULL,
            1,
            1
        },
        { 
            "report",
            "After-build report",
            NULL,
            printOtherReport,
            access(BUILD_REPORT_PATH, F_OK) == 0
        },
        { 
            "support",
            "Getting support or involved",
            NULL,
            printOtherSupport,
            1
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
    freeMenu(rawMenu, rawMenuSize);

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

    freeMenu(menu, menuSize);
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
        menu[i].payload = NULL;
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
                printSoftwareLicence((cursorY - 1) + (cursorX - 1) * rows);
                break;
        
            case QUIT:
                running = 0;
                break;

            case INVALID:
                fullRedraw = 0;
                break;
        }
    }

    freeMenu(menu, LICENCES_NO);
    clearScreen();
}
