/*
    ######################################################
    ##            SHORK UTILITY - SHORKHELP             ##
    ######################################################
    ## A utility for SHORK 486 that informs about its   ##
    ## capabilities and provides guidance               ##
    ######################################################
    ## Licence: GNU GENERAL PUBLIC LICENSE Version 3    ##
    ######################################################
    ## Kali (sharktastica.co.uk)                        ##
    ######################################################
*/



#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>



#define COL_BLACK          "0;30"
#define COL_BLUE           "0;34"
#define COL_BRIGHT_BLACK   "1;30"
#define COL_BRIGHT_BLUE    "1;34"
#define COL_BRIGHT_CYAN    "1;36"
#define COL_BRIGHT_GREEN   "1;32"
#define COL_BRIGHT_MAGENTA "1;35"
#define COL_BRIGHT_RED     "1;31"
#define COL_BRIGHT_WHITE   "1;37"
#define COL_BRIGHT_YELLOW  "1;33"
#define COL_CYAN           "0;36"
#define COL_GREEN          "0;32"
#define COL_MAGENTA        "0;35"
#define COL_RED            "0;31"
#define COL_WHITE          "0;37"
#define COL_YELLOW         "0;33"

static int colReset = 0;
static struct winsize termSize;



/**
 * Moves the cursor to topleft-most position and clears below cursor.
 */
void clearScreen(void)
{
    system("clear");
}

/**
 * Finds and replaces a given search term with a desired replacement term from an
 * input string.
 * @param buffer Input string
 * @param bufferSize Size to use when allocating the result string
 * @param needle Substring to find and replace
 * @param replacement New string to insert
 * @return String after term replacement
 */
char *findReplace(const char *buffer, const size_t bufferSize, const char *needle, const char *replacement)
{
    if (!buffer || !needle || !replacement || bufferSize < 2) return strdup("");

    size_t needleLen = strlen(needle);
    size_t replacementLen = strlen(replacement);
    if (needleLen == 0) return strdup("");

    char *result = malloc(bufferSize);
    if (!result) return strdup("");

    strncpy(result, buffer, bufferSize);
    result[bufferSize - 1] = '\0';

    char *pos = result;
    while ((pos = strstr(pos, needle)) != NULL)
    {
        size_t tailLen = strlen(pos + needleLen);

        if (replacementLen > needleLen)
        {
            size_t currentLen = strlen(result);
            size_t newLen = currentLen + (replacementLen - needleLen) + 1;
            char *tmp = realloc(result, newLen);
            if (!tmp) break;
            pos = tmp + (pos - result);
            result = tmp;
        }

        memmove(pos + replacementLen, pos + needleLen, tailLen + 1);
        memcpy(pos, replacement, replacementLen);
        pos += replacementLen;
    }

    return result;
}

/**
 * Adds new lines to a given string based on the requested line width.
 * @param buffer Input string
 * @param width Characters per line
 * @param indent Indent to include after newly inserted new line
 * @return Number of lines in the string
 */
int formatNewLines(char *buffer, int width, char *indent)
{
    if (!buffer || width < 1) return 0;

    size_t bufferStrLen = strlen(buffer);
    size_t indentLen = indent ? strlen(indent) : 0;
    int lines = 1;
    int lastSpace = -1;
    int widthCount = 1;

    for (int i = 0; i < bufferStrLen; i++)
    {
        if (buffer[i] == '\033')
        {
            while (i < bufferStrLen && buffer[i] != 'm') i++;
            if (i >= bufferStrLen) break;
            continue; 
        }
        
        if (buffer[i] == ' ') lastSpace = i;
        else if (buffer[i] == '\n')
        {
            lines++;
            widthCount = 0;
            continue;
        }

        if (widthCount == width)
        {
            if (lastSpace != -1)
            {
                buffer[lastSpace] = '\n';
                lines++;

                if (indent && indentLen > 0)
                {
                    memmove(buffer + lastSpace + 1 + indentLen, buffer + lastSpace + 1, bufferStrLen - lastSpace);
                    memcpy(buffer + lastSpace + 1, indent, indentLen);
                    bufferStrLen += indentLen;
                    if (lastSpace <= i) i += indentLen;
                }
            }
            widthCount = i - lastSpace;
        }

        widthCount++;
    }

    return lines;
}

/**
 * @return winsize struct containing the current terminal size in columns and rows
 */
struct winsize getTerminalSize(void)
{
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0)
    {
        ws.ws_col = 80;
        ws.ws_row = 24;
    }
    return ws;
}

int isProgramInstalled(const char *prog)
{
    char *path = getenv("PATH");
    if (!path)
    {
        char cmd[64];
        snprintf(cmd, 64, "%s --version > /dev/null 2>&1", prog);
        return (system(cmd) == 0);
    }

    char *paths = strdup(path);
    char *dir = strtok(paths, ":");
    while (dir)
    {
        char fullpath[512];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", dir, prog);
        if (access(fullpath, X_OK) == 0)
        {
            free(paths);
            return 1;
        }
        dir = strtok(NULL, ":");
    }
    free(paths);
    return 0;
}

void printCommands(void)
{
    clearScreen();

    char networkingStr[200];
    if (isProgramInstalled("ip"))
    {
        snprintf(networkingStr, 200, "\n\033[%smNetworking & remote access\033[%sm\nftp, ftpget, ftpput, ip, ifconfig, ping, route, scp, ssh, telnet, traceroute, udhcpc, wget, whois\n", COL_BRIGHT_CYAN, COL_WHITE);

        if (!isProgramInstalled("ftp"))
        {
            char *tmp = findReplace(networkingStr, 200, "ftp, ", "");
            strncpy(networkingStr, tmp, 199);
            networkingStr[199] = '\0';
            free(tmp);
        }
        if (!isProgramInstalled("scp"))
        {
            char *tmp = findReplace(networkingStr, 200, "scp, ", "");
            strncpy(networkingStr, tmp, 199);
            networkingStr[199] = '\0';
            free(tmp);
        }
        if (!isProgramInstalled("ssh"))
        {
            char *tmp = findReplace(networkingStr, 200, "ssh, ", "");
            strncpy(networkingStr, tmp, 199);
            networkingStr[199] = '\0';
            free(tmp);
        }
    }

    char commandsStr[1500];
    snprintf(commandsStr, 1500, 
"\033[%smGeneral\033[%sm\n\
ar, arch, ascii, awk, basename, bc, cal, cat, chmod, chown, chroot, chvt, cp, cut, dc, dirname, eject, expand, expr, false, file, find, fold, grep, gzip, head, less, ln, ls, man, mkdir, mv, paste, printf, pwd, readlink, rev, rm, rmdir, sed, seq, setfont, showkey, stat, tar, tee, test, touch, tr, tree, true, truncate, unexpand, unzip, usleep, volname, wc, whereis, which, xz, yes\n\n\
\033[%smEditors & development tools\033[%sm\n\
as, ed, emacs, g++, gcc, git, gfortran, hexdump, nano, tcc, vi, xxd\n\n\
\033[%smSystem & processes\033[%sm\n\
beep, blkid, clear, crontab, date, dd, df, dmesg, du, fdformat, fdisk, free, halt, hostname, kill, killall, loadkmap, losetup, lsblk, lspci, lsusb, mdev, mknod, mount, mountpoint, nice, nohup, nproc, partprobe, partx, pkill, ps, pstree, sfdisk, sleep, strace, stty, swapoff, swapon, sync, taskset, top, umount, uname, whoami\n%s",
    COL_BRIGHT_CYAN, COL_WHITE, COL_BRIGHT_CYAN, COL_WHITE, COL_BRIGHT_CYAN, COL_WHITE, networkingStr);

    if (!isProgramInstalled("as"))
    {
        char *tmp = findReplace(commandsStr, 1500, "as, ", "");
        strncpy(commandsStr, tmp, 1499);
        commandsStr[1499] = '\0';
        free(tmp);
    }
    if (!isProgramInstalled("emacs"))
    {
        char *tmp = findReplace(commandsStr, 1500, "emacs, ", "");
        strncpy(commandsStr, tmp, 1499);
        commandsStr[1499] = '\0';
        free(tmp);
    }
    if (!isProgramInstalled("file"))
    {
        char *tmp = findReplace(commandsStr, 1500, "file, ", "");
        strncpy(commandsStr, tmp, 1499);
        commandsStr[1499] = '\0';
        free(tmp);
    }
    if (!isProgramInstalled("g++"))
    {
        char *tmp = findReplace(commandsStr, 1500, "g++, ", "");
        strncpy(commandsStr, tmp, 1499);
        commandsStr[1499] = '\0';
        free(tmp);
    }
    if (!isProgramInstalled("gcc"))
    {
        char *tmp = findReplace(commandsStr, 1500, "gcc, ", "");
        strncpy(commandsStr, tmp, 1499);
        commandsStr[1499] = '\0';
        free(tmp);
    }
    if (!isProgramInstalled("gfortran"))
    {
        char *tmp = findReplace(commandsStr, 1500, "gfortran, ", "");
        strncpy(commandsStr, tmp, 1499);
        commandsStr[1499] = '\0';
        free(tmp);
    }
    if (!isProgramInstalled("lsusb"))
    {
        char *tmp = findReplace(commandsStr, 1500, "lsusb, ", "");
        strncpy(commandsStr, tmp, 1499);
        commandsStr[1499] = '\0';
        free(tmp);
    }
    if (!isProgramInstalled("nano"))
    {
        char *tmp = findReplace(commandsStr, 1500, "nano, ", "");
        strncpy(commandsStr, tmp, 1499);
        commandsStr[1499] = '\0';
        free(tmp);
    }
    if (!isProgramInstalled("i386-tcc") && !isProgramInstalled("tcc"))
    {
        char *tmp = findReplace(commandsStr, 1500, "tcc, ", "");
        strncpy(commandsStr, tmp, 1499);
        commandsStr[1499] = '\0';
        free(tmp);
    }

    formatNewLines(commandsStr, termSize.ws_col, NULL);
    printf("%s", commandsStr);
}

void printEmacsCheatsheet(void)
{
    clearScreen();

    char emacsStr[1500];
    snprintf(emacsStr, 1500, 
"\033[%smEmacs (Mg) cheatsheet\n\n\
\033[%smC\033[%sm = Ctrl                          \033[%smM\033[%sm = Alt\n\n\
\033[%smExit\033[%sm                   \033[%smC\033[%sm-x \033[%smC\033[%sm-c    \033[%smOpen file/new buffer\033[%sm      \033[%smC\033[%sm-x \033[%smC\033[%sm-f\n\
\033[%smSave current buffer\033[%sm    \033[%smC\033[%sm-x \033[%smC\033[%sm-s    \033[%smSave all buffers\033[%sm          \033[%smC\033[%sm-x s\n\n\
\033[%smSplit window\033[%sm           \033[%smC\033[%sm-x 2      \033[%smEnlarge window\033[%sm            \033[%smC\033[%sm-x ^\n\
\033[%smSwitch windows\033[%sm         \033[%smC\033[%sm-x o      \033[%smClose current window\033[%sm      \033[%smC\033[%sm-x 0\n\
\033[%smClose other windows\033[%sm    \033[%smC\033[%sm-x 1\n\n\
\033[%smGo to line\033[%sm             \033[%smC\033[%sm-x g      \033[%smUndo\033[%sm                      \033[%smC\033[%sm-_\n\
\033[%smSearch (forwards)\033[%sm      \033[%smC\033[%sm-s        \033[%smSearch (backwards)\033[%sm        \033[%smC\033[%sm-r\n\
\033[%smFind and replace\033[%sm       \033[%smM\033[%sm-%%        \033[%smReformat (fit to line)\033[%sm    \033[%smM\033[%sm-q\n",
    COL_BRIGHT_CYAN, COL_RED, COL_WHITE, COL_MAGENTA, COL_WHITE,
    COL_GREEN, COL_WHITE, COL_RED, COL_WHITE, COL_RED, COL_WHITE, COL_GREEN, COL_WHITE, COL_RED, COL_WHITE, COL_RED, COL_WHITE,
    COL_GREEN, COL_WHITE, COL_RED, COL_WHITE, COL_RED, COL_WHITE, COL_GREEN, COL_WHITE, COL_RED, COL_WHITE,
    COL_GREEN, COL_WHITE, COL_RED, COL_WHITE, COL_GREEN, COL_WHITE, COL_RED, COL_WHITE,
    COL_GREEN, COL_WHITE, COL_RED, COL_WHITE, COL_GREEN, COL_WHITE, COL_RED, COL_WHITE,
    COL_GREEN, COL_WHITE, COL_RED, COL_WHITE,
    COL_GREEN, COL_WHITE, COL_RED, COL_WHITE, COL_GREEN, COL_WHITE, COL_RED, COL_WHITE,
    COL_GREEN, COL_WHITE, COL_RED, COL_WHITE, COL_GREEN, COL_WHITE, COL_RED, COL_WHITE,
    COL_GREEN, COL_WHITE, COL_MAGENTA, COL_WHITE, COL_GREEN, COL_WHITE, COL_MAGENTA, COL_WHITE);

    formatNewLines(emacsStr, termSize.ws_col, NULL);
    printf("%s", emacsStr);
}

void printGitCommands(void)
{
    clearScreen();

    char gitStr[500];
    snprintf(gitStr, 500, 
"\033[%smSupported Git commands\033[%sm\n\n\
add, blame, branch, checkout, cherry-pick, clean, clone, commit, config, diff, fetch, init, log, merge, mv, pull, push, rebase, reflog, remote, reset, restore, rm, show, stash, status, switch, tag\n",
    COL_BRIGHT_CYAN, COL_WHITE);

    formatNewLines(gitStr, termSize.ws_col, NULL);
    printf("%s", gitStr);
}

void printIntro(void)
{
    clearScreen();

    char introStr[400];
    snprintf(introStr, 400, "\033[%smSHORK 486 is a minimal Linux distribution for 486 and Pentium (P5) PCs! It focuses on being as lean and small as possible, whilst still providing a robust command and utilities set, and including hand-picked modern and custom bundled software. The goal is to provide an alternative use for old PCs, hopefully saving one or two of them from landfills.\n\n", COL_WHITE);
    formatNewLines(introStr, termSize.ws_col, NULL);
    printf("%s", introStr);

    char gettingStartedStr[1200];
    snprintf(gettingStartedStr, 1200, 
"\033[%smGetting started\n\
\033[%sm1.\033[%sm Set your keyboard's layout with \033[%smshorkmap\033[%sm.\n\
\033[%sm2.\033[%sm Pick a font and colour for the console terminal with \033[%smshorkfont\033[%sm.\n\
\033[%sm3.\033[%sm (If compatible) Change your display's resolution with \033[%smshorkres\033[%sm. A reboot will be required.\n\
\033[%sm4.\033[%sm Set your computer's name by editing \033[%sm/etc/hostname\033[%sm. A reboot will be required, or you can run: \033[%smhostname \"$(cat /etc/hostname)\"\n\
\033[%sm5.\033[%sm (If applicable) Test your network connection with \033[%smping\033[%sm. For example: \033[%smping sharktastica.co.uk\n\
\033[%sm6.\033[%sm Run \033[%smshorkfetch\033[%sm to see a quick overview of your system and environment.\n\
\033[%sm7.\033[%sm Check \033[%smshorkhelp\033[%sm's other options to learn what commands and software are available, and see what guides may be of use.\n\
\033[%sm8.\033[%sm Use Ctrl+Alt+F1/F2/F3 or \033[%smchvt\033[%sm to switch between the three available virtual consoles, useful for multitasking, troubleshooting and recovery.\n\
\033[%sm9.\033[%sm When you are finished using SHORK 486, run \033[%smshorkoff\033[%sm to safely halt the system before powering off.\n",
    COL_BRIGHT_CYAN,
    COL_GREEN, COL_WHITE, COL_BRIGHT_MAGENTA, COL_WHITE,
    COL_GREEN, COL_WHITE, COL_BRIGHT_MAGENTA, COL_WHITE,
    COL_GREEN, COL_WHITE, COL_BRIGHT_MAGENTA, COL_WHITE,
    COL_GREEN, COL_WHITE, COL_BRIGHT_RED, COL_WHITE, COL_BRIGHT_RED,
    COL_GREEN, COL_WHITE, COL_BRIGHT_RED, COL_WHITE, COL_BRIGHT_RED,
    COL_GREEN, COL_WHITE, COL_BRIGHT_MAGENTA, COL_WHITE,
    COL_GREEN, COL_WHITE, COL_BRIGHT_MAGENTA, COL_WHITE,
    COL_GREEN, COL_WHITE, COL_BRIGHT_RED, COL_WHITE,
    COL_GREEN, COL_WHITE, COL_BRIGHT_MAGENTA, COL_WHITE);

    formatNewLines(gettingStartedStr, termSize.ws_col, "   ");
    printf("%s", gettingStartedStr);
}

void printShorkUtilities(void)
{
    clearScreen();

    char utilsStr[1500];
    snprintf(utilsStr, 1500, 
"\033[%smSHORK Utilities\n\n\
\033[%smshorkdir\033[%sm: Terminal-based file browser and file inspector (if file is installed).\n\n\
\033[%smshorkfetch\033[%sm: Displays basic system and environment information. Similar to fastfetch, neofetch, etc.\n\n\
\033[%smshorkfont\033[%sm: Changes the terminal's font or colour. Takes two arguments (type of change and name); no arguments shows how to use and a list of possible colours.\n\n\
\033[%smshorkhelp\033[%sm: Provides help with using SHORK 486 via command lists, guides and cheatsheets. Requires the use of one of five parameters.\n\n\
\033[%smshorkmap\033[%sm: Changes the system's keyboard map. Takes takes one argument (a keymap name); no arguments show a list of possible keymaps.\n\n\
\033[%smshorkoff\033[%sm: Brings the system to a halt and syncs the write cache, allowing the computer to be safely turned off. Similar to poweroff or shutdown -h.\n\n\
\033[%smshorkres\033[%sm: Changes the system's display resolution (provided hardware is compatible). Takes one argument (a resolution name); no arguments show a list of possible resolution names.\n",
    COL_BRIGHT_CYAN, COL_BRIGHT_MAGENTA, COL_WHITE, COL_BRIGHT_MAGENTA, COL_WHITE, COL_BRIGHT_MAGENTA, COL_WHITE, COL_BRIGHT_MAGENTA, COL_WHITE, COL_BRIGHT_MAGENTA, COL_WHITE, COL_BRIGHT_MAGENTA, COL_WHITE, COL_BRIGHT_MAGENTA, COL_WHITE);

    formatNewLines(utilsStr, termSize.ws_col, NULL);
    printf("%s", utilsStr);
}

void showArgumentsList(void)
{
    char cmdDesc[90] = "Displays help and reference information for using SHORK 486 and its built-in tools.\n\n";
    formatNewLines(cmdDesc, termSize.ws_col, NULL);
    printf("%s\n", cmdDesc);

    char usage[80] = "Usage: shorkhelp {--commands | --emacs | --git | --intro | --shorkutils}\n";

    if (!isProgramInstalled("mg"))
    {
        char *tmp = findReplace(usage, 80, " | --emacs ", " ");
        strncpy(usage, tmp, 79);
        usage[79] = '\0';
        free(tmp);
    }

    if (!isProgramInstalled("git"))
    {
        char *tmp = findReplace(usage, 80, " | --git ", " ");
        strncpy(usage, tmp, 79);
        usage[79] = '\0';
        free(tmp);
    }

    formatNewLines(usage, termSize.ws_col, NULL);
    printf("%s", usage);
}



int main(int argc, char *argv[])
{
    termSize = getTerminalSize();

    if (argc == 1 || argc > 2)
        showArgumentsList();
    else if (argc == 2)
    {
        if (strcmp(argv[1], "--commands") == 0)
            printCommands();
        else if (strcmp(argv[1], "--emacs") == 0 || strcmp(argv[1], "--mg") == 0)
            printEmacsCheatsheet();
        else if (strcmp(argv[1], "--git") == 0)
            printGitCommands();
        else if (strcmp(argv[1], "--intro") == 0)
            printIntro();
        else if (strcmp(argv[1], "--shorkutils") == 0)
            printShorkUtilities();
        else
            showArgumentsList();
    }

    return 0;
}
