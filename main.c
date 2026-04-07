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



#include <ctype.h>
#include <sys/ioctl.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>



enum NavInput 
{
    CURSOR_DOWN,
    CURSOR_UP,
    CURSOR_LEFT,
    CURSOR_RIGHT,
    QUIT,
    ENTER,
    INVALID
};

typedef struct 
{
    char *id;
    char *name;
    void (*action)(void);
    int visible;
} MenuItem;



#define COL_BAK_BLACK           "40"
#define COL_BAK_BLUE            "44"
#define COL_BAK_CYAN            "46"
#define COL_BAK_GREEN           "42"
#define COL_BAK_GREY            "40"
#define COL_BAK_MAGENTA         "45"
#define COL_BAK_RED             "41"
#define COL_BAK_WHITE           "47"
#define COL_BAK_YELLOW          "43"

#define COL_FOR_BLACK           "0;30"
#define COL_FOR_BLUE            "0;34"
#define COL_FOR_BOLD_BLUE       "1;34"
#define COL_FOR_BOLD_CYAN       "1;36"
#define COL_FOR_BOLD_GREEN      "1;32"
#define COL_FOR_BOLD_MAGENTA    "1;35"
#define COL_FOR_BOLD_RED        "1;31"
#define COL_FOR_BOLD_WHITE      "1;37"
#define COL_FOR_BOLD_YELLOW     "1;33"
#define COL_FOR_CYAN            "0;36"
#define COL_FOR_GREEN           "0;32"
#define COL_FOR_GREY            "1;30"
#define COL_FOR_MAGENTA         "0;35"
#define COL_FOR_RED             "0;31"
#define COL_FOR_WHITE           "0;37"
#define COL_FOR_YELLOW          "0;33"

#define COL_RESET               "0"
#define COL_FOR_RESET           "39"
#define COL_BAK_RESET           "49"

static int COL_ENABLED = 1;
static char *COL_FOR_ARROW = COL_FOR_BOLD_RED;
static char *COL_FOR_CODE = COL_FOR_BOLD_RED;
static char *COL_FOR_CURSOR = COL_FOR_BOLD_CYAN;
static char *COL_FOR_HEADING = COL_FOR_BOLD_CYAN;
static char *COL_FOR_OL = COL_FOR_GREEN;
static struct termios OLD_TERMIOS;
static struct winsize TERM_SIZE;



/**
 * Awaits for any user input.
 */
void awaitInput(void)
{
    printf("\033[%s;%sm", COL_FOR_BOLD_WHITE, COL_BAK_BLUE);
    int len = printf("Press any key to continue... ");
    if (COL_ENABLED)
        for (size_t i = len; i < TERM_SIZE.ws_col; i++)
            printf(" ");
    getchar();
    printf("\033[%sm", COL_RESET);
}

/**
 * Moves the cursor to topleft-most position and clears below cursor.
 */
void clearScreen(void)
{
    printf("\033[H\033[J");
}

/**
 * Enables the terminal's canonical input. Used only when the program exits.
 */
void disableRawMode(void)
{
    tcsetattr(STDIN_FILENO, TCSANOW, &OLD_TERMIOS);
}

/**
 * Disables the terminal's canonical input so that things like getchar do not
 * wait until enter is pressed.
 */
void enableRawMode(void)
{
    struct termios newTERMIO;
    tcgetattr(STDIN_FILENO, &OLD_TERMIOS);
    newTERMIO = OLD_TERMIOS;
    newTERMIO.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newTERMIO);
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
}

/**
 * @return The nav input action detected
 */
enum NavInput getNavInput(void)
{
    int c = getchar();

    if (c == 27)
    {
        getchar();
        switch (getchar())
        {
            case 'A': return CURSOR_UP;
            case 'B': return CURSOR_DOWN;
            case 'C': return CURSOR_RIGHT;
            case 'D': return CURSOR_LEFT;
        }
    }
    else
    {
        c = tolower(c);
        switch (c)
        {
            case '\n': return ENTER;
            case '\r': return ENTER;
            case 'q': return QUIT;
            case 'w': return CURSOR_UP;
            case 'a': return CURSOR_LEFT;
            case 's': return CURSOR_DOWN;
            case 'd': return CURSOR_RIGHT;
            case 'h': return CURSOR_LEFT;
            case 'j': return CURSOR_DOWN;
            case 'k': return CURSOR_UP;
            case 'l': return CURSOR_RIGHT;
        }
    }

    return INVALID;
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

    // Hard-coded check in /usr/libexec for shorkfont 
    char libexecPath[PATH_MAX];
    snprintf(libexecPath, PATH_MAX, "/usr/libexec/%s", prog);
    if (access(libexecPath, X_OK) == 0) return 1;

    return 0;
}

/**
 * @param caption Caption to be printed in the footer
 */
void printFooter(char *caption)
{
    if (COL_ENABLED)
        printf("\033[%s;%sm", COL_FOR_BOLD_WHITE, COL_BAK_BLUE);
    else
        for (int i = 0; i < TERM_SIZE.ws_col; i++) printf("-");

    int len = printf("%s", caption);

    if (COL_ENABLED)
    {
        for (int i = len; i < TERM_SIZE.ws_col; i++) printf(" ");
        printf("\033[%sm", COL_RESET);
    }
}

/**
 * @param title Title to be printed in the header
 */
void printHeader(char *title)
{
    if (COL_ENABLED)
        printf("\033[%s;%sm", COL_FOR_BOLD_WHITE, COL_BAK_BLUE);

    size_t titleLen = strlen(title);
    if (titleLen <= TERM_SIZE.ws_col) 
    {
        printf("%s", title);
        if (COL_ENABLED)
            for (size_t i = titleLen; i < TERM_SIZE.ws_col; i++)
                printf(" ");
        else
            printf("\n");
    }
    else
    {
        size_t visibleLen = TERM_SIZE.ws_col - 3;
        char *start = title + (titleLen - visibleLen);
        printf("...%s\n", start);
    }

    if (COL_ENABLED)
        printf("\033[%sm", COL_RESET);
    else
        for (int i = 0; i < TERM_SIZE.ws_col; i++) printf("-");
}

void printCommands(void)
{
    printHeader("Commands & programs list");

    char networkingStr[200];
    if (isProgramInstalled("ip"))
    {
        snprintf(networkingStr, 200, "\n\033[%smNetworking & remote access\033[%sm\nftp, ftpget, ftpput, ip, ifconfig, ping, route, scp, ssh, telnet, traceroute, udhcpc, wget, whois\n", COL_FOR_HEADING, COL_FOR_WHITE);

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
    COL_FOR_HEADING, COL_FOR_WHITE, COL_FOR_HEADING, COL_FOR_WHITE, COL_FOR_HEADING, COL_FOR_WHITE, networkingStr);

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

    formatNewLines(commandsStr, TERM_SIZE.ws_col, NULL);
    printf("%s", commandsStr);
}

void printEmacsCheatsheet(void)
{
    printHeader("Emacs (Mg) cheatsheet");

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
    COL_FOR_RED, COL_FOR_WHITE, COL_FOR_MAGENTA, COL_FOR_WHITE,
    COL_FOR_OL, COL_FOR_WHITE, COL_FOR_RED, COL_FOR_WHITE, COL_FOR_RED, COL_FOR_WHITE, COL_FOR_OL, COL_FOR_WHITE, COL_FOR_RED, COL_FOR_WHITE, COL_FOR_RED, COL_FOR_WHITE,
    COL_FOR_OL, COL_FOR_WHITE, COL_FOR_RED, COL_FOR_WHITE, COL_FOR_RED, COL_FOR_WHITE, COL_FOR_OL, COL_FOR_WHITE, COL_FOR_RED, COL_FOR_WHITE,
    COL_FOR_OL, COL_FOR_WHITE, COL_FOR_RED, COL_FOR_WHITE, COL_FOR_OL, COL_FOR_WHITE, COL_FOR_RED, COL_FOR_WHITE,
    COL_FOR_OL, COL_FOR_WHITE, COL_FOR_RED, COL_FOR_WHITE, COL_FOR_OL, COL_FOR_WHITE, COL_FOR_RED, COL_FOR_WHITE,
    COL_FOR_OL, COL_FOR_WHITE, COL_FOR_RED, COL_FOR_WHITE,
    COL_FOR_OL, COL_FOR_WHITE, COL_FOR_RED, COL_FOR_WHITE, COL_FOR_OL, COL_FOR_WHITE, COL_FOR_RED, COL_FOR_WHITE,
    COL_FOR_OL, COL_FOR_WHITE, COL_FOR_RED, COL_FOR_WHITE, COL_FOR_OL, COL_FOR_WHITE, COL_FOR_RED, COL_FOR_WHITE,
    COL_FOR_OL, COL_FOR_WHITE, COL_FOR_MAGENTA, COL_FOR_WHITE, COL_FOR_OL, COL_FOR_WHITE, COL_FOR_MAGENTA, COL_FOR_WHITE);

    formatNewLines(emacsStr, TERM_SIZE.ws_col, NULL);
    printf("%s", emacsStr);
}

void printGitCommands(void)
{
    printHeader("Supported Git commands");

    char gitStr[500];
    snprintf(gitStr, 500, "add, blame, branch, checkout, cherry-pick, clean, clone, commit, config, diff, fetch, init, log, merge, mv, pull, push, rebase, reflog, remote, reset, restore, rm, show, stash, status, switch, tag\n");

    formatNewLines(gitStr, TERM_SIZE.ws_col, NULL);
    printf("%s", gitStr);
}

void printIntro(void)
{
    printHeader("Introduction to SHORK 486");

    char introStr[400];
    snprintf(introStr, 400, "\033[%smSHORK 486 is a minimal Linux distribution for 486 and Pentium (P5) PCs! It focuses on being as lean and small as possible, whilst still providing a robust command and utilities set, and including hand-picked modern and custom bundled software. The goal is to provide an alternative use for old PCs, hopefully saving one or two of them from landfills.\n\n", COL_FOR_WHITE);
    formatNewLines(introStr, TERM_SIZE.ws_col, NULL);
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
    COL_FOR_HEADING,
    COL_FOR_OL, COL_FOR_WHITE, COL_FOR_BOLD_MAGENTA, COL_FOR_WHITE,
    COL_FOR_OL, COL_FOR_WHITE, COL_FOR_BOLD_MAGENTA, COL_FOR_WHITE,
    COL_FOR_OL, COL_FOR_WHITE, COL_FOR_BOLD_MAGENTA, COL_FOR_WHITE,
    COL_FOR_OL, COL_FOR_WHITE, COL_FOR_CODE, COL_FOR_WHITE, COL_FOR_CODE,
    COL_FOR_OL, COL_FOR_WHITE, COL_FOR_CODE, COL_FOR_WHITE, COL_FOR_CODE,
    COL_FOR_OL, COL_FOR_WHITE, COL_FOR_BOLD_MAGENTA, COL_FOR_WHITE,
    COL_FOR_OL, COL_FOR_WHITE, COL_FOR_BOLD_MAGENTA, COL_FOR_WHITE,
    COL_FOR_OL, COL_FOR_WHITE, COL_FOR_CODE, COL_FOR_WHITE,
    COL_FOR_OL, COL_FOR_WHITE, COL_FOR_BOLD_MAGENTA, COL_FOR_WHITE);

    formatNewLines(gettingStartedStr, TERM_SIZE.ws_col, "   ");
    printf("%s", gettingStartedStr);
}

void printMenu(MenuItem *menu, int menuSize, int cursor, int cursorPrev)
{
    int baseRow = 2;
    int availHeight = TERM_SIZE.ws_row - 2;
    if (!COL_ENABLED)
    {
        baseRow = 3;
        availHeight = TERM_SIZE.ws_row - 4;
    }

    // Viewport offset and clamping for current line cursor
    int offset = (cursor - 1) - (availHeight / 2);
    if (offset < 0) offset = 0;
    if (offset > menuSize - availHeight) offset = menuSize - availHeight;
    if (offset < 0) offset = 0;

    // Viewport offset and clamping for previous line cursor
    int prevOffset = (cursorPrev - 1) - (availHeight / 2);
    if (prevOffset < 0) prevOffset = 0;
    if (prevOffset > menuSize - availHeight) prevOffset = menuSize - availHeight;
    if (prevOffset < 0) prevOffset = 0;

    int inScrolling = (prevOffset != offset);

    // cursorPrev is initialised as 0 to indicate first frame should be drawn
    if (cursorPrev == 0) inScrolling = 1;

    int prevIndex = cursorPrev - 1;
    int currIndex = cursor - 1;

    // If we don't need to scroll, just update the cursor
    if (!inScrolling)
    {
        int rowPrev = baseRow + (prevIndex - offset);
        int rowCurr = baseRow + (currIndex - offset);

        // Remove old line cursor
        printf("\x1b[%d;1H[ ]", rowPrev);

        // Print new line cursor
        printf("\x1b[%d;1H[\033[%sm*\033[%sm]", rowCurr, COL_FOR_CURSOR, COL_RESET);

        // DEBUG
        //printf("\x1b[1;%dH1", TERM_SIZE.ws_col);
        return;
    }

    // DEBUG
    //printf("\x1b[1;%dH0", TERM_SIZE.ws_col);

    int canGoUp = offset > 0;
    int canGoDown = (offset + availHeight) < menuSize;
    int linesPrinted = 0;

    for (int i = offset; i < menuSize && i < offset + availHeight; i++)
    {
        printf("\x1b[%d;1H\x1b[K", baseRow + linesPrinted);

        // Can scroll up indicator
        if (canGoUp && i == offset)
            printf("\033[%sm^\033[%sm\x1b[K\n", COL_FOR_ARROW, COL_RESET);
        // Can scroll down indicator
        else if (canGoDown && i == offset + availHeight - 1)
            printf("\033[%smv\033[%sm\n", COL_FOR_ARROW, COL_RESET);
        // Selected line
        else if (i == currIndex)
            printf("[\033[%sm*\033[%sm] %s\n", COL_FOR_CURSOR, COL_RESET, menu[i].name);
        // Other lines
        else
            printf("[ ] %s\n", menu[i].name);

        linesPrinted++;
    }

    // "Fill in" lines if listing is shorter than viewport
    if (!canGoUp && !canGoDown)
        for (int i = linesPrinted; i < availHeight; i++)
            printf("\n");
}

void printSHORKEntertainment(void)
{
    printHeader("SHORK Entertainment");

    if (isProgramInstalled("sl"))
    {
        char shorklocomotiveStr[180];
        snprintf(shorklocomotiveStr, 180, "\033[%smshorklocomotive\033[%sm: A shark-themed take on the \033[%smsl\033[%sm command that kindly pokes fun at making typos when trying to type \033[%smls\033[%sm.\n", COL_FOR_BOLD_MAGENTA, COL_FOR_WHITE, COL_FOR_CODE, COL_FOR_WHITE, COL_FOR_CODE, COL_FOR_WHITE);
        formatNewLines(shorklocomotiveStr, TERM_SIZE.ws_col, "    ");
        printf("%s", shorklocomotiveStr);
    }

    if (isProgramInstalled("shorksay"))
    {
        char shorksayStr[190];
        snprintf(shorksayStr, 190, "\033[%smshorksay\033[%sm: A shark-themed take on the \033[%smcowsay\033[%sm command that outputs an ASCII art shark and speech bubble containing a message of your choice.\n", COL_FOR_BOLD_MAGENTA, COL_FOR_WHITE, COL_FOR_CODE, COL_FOR_WHITE);
        formatNewLines(shorksayStr, TERM_SIZE.ws_col, "    ");
        printf("%s", shorksayStr);
    }
}

void printSHORKUtilities(void)
{
    printHeader("SHORK Utilities");

    if (isProgramInstalled("shorkdir"))
    {
        char shorkdirStr[130];
        snprintf(shorkdirStr, 130, "\033[%smshorkdir\033[%sm: Terminal-based file browser and file inspector (if \033[%smfile\033[%sm is installed).\n", COL_FOR_BOLD_MAGENTA, COL_FOR_WHITE, COL_FOR_CODE, COL_FOR_WHITE);
        formatNewLines(shorkdirStr, TERM_SIZE.ws_col, "    ");
        printf("%s", shorkdirStr);
    }

    if (isProgramInstalled("shorkfetch"))
    {
        char shorkfetchStr[160];
        snprintf(shorkfetchStr, 160, "\033[%smshorkfetch\033[%sm: Displays basic system and environment information. Similar to \033[%smfastfetch\033[%sm, \033[%smneofetch\033[%sm, etc.\n", COL_FOR_BOLD_MAGENTA, COL_FOR_WHITE, COL_FOR_CODE, COL_FOR_WHITE, COL_FOR_CODE, COL_FOR_WHITE);
        formatNewLines(shorkfetchStr, TERM_SIZE.ws_col, "    ");
        printf("%s", shorkfetchStr);
    }

    if (isProgramInstalled("shorkfont"))
    {
        char shorkfontStr[190];
        snprintf(shorkfontStr, 190, "\033[%smshorkfont\033[%sm: Changes the terminal's font or colour. Takes two arguments (type of change and name); no arguments shows how to use and a list of possible colours.\n", COL_FOR_BOLD_MAGENTA, COL_FOR_WHITE);
        formatNewLines(shorkfontStr, TERM_SIZE.ws_col, "    ");
        printf("%s", shorkfontStr);
    }

    char shorkhelpStr[170];
    snprintf(shorkhelpStr, 170, "\033[%smshorkhelp\033[%sm: Provides help with using SHORK 486 via command lists, guides and cheatsheets. Requires the use of one of five parameters.\n", COL_FOR_BOLD_MAGENTA, COL_FOR_WHITE);
    formatNewLines(shorkhelpStr, TERM_SIZE.ws_col, "    ");
    printf("%s", shorkhelpStr);

    if (isProgramInstalled("shorkmap"))
    {
        char shorkmapStr[170];
        snprintf(shorkmapStr, 170, "\033[%smshorkmap\033[%sm: Changes the system's keyboard map. Takes takes one argument (a keymap name); no arguments show a list of possible keymaps.\n", COL_FOR_BOLD_MAGENTA, COL_FOR_WHITE);
        formatNewLines(shorkmapStr, TERM_SIZE.ws_col, "    ");
        printf("%s", shorkmapStr);
    }

    if (isProgramInstalled("shorkoff"))
    {
        char shorkoffStr[210];
        snprintf(shorkoffStr, 210, "\033[%smshorkoff\033[%sm: Brings the system to a halt and syncs the write cache, allowing the computer to be safely turned off. Similar to \033[%smpoweroff\033[%sm or \033[%smshutdown -h\033[%sm.\n", COL_FOR_BOLD_MAGENTA, COL_FOR_WHITE, COL_FOR_CODE, COL_FOR_WHITE, COL_FOR_CODE, COL_FOR_WHITE);
        formatNewLines(shorkoffStr, TERM_SIZE.ws_col, "    ");
        printf("%s", shorkoffStr);
    }

    if (isProgramInstalled("shorkres"))
    {
        char shorkresStr[220];
        snprintf(shorkresStr, 220, "\033[%smshorkres\033[%sm: Changes the system's display resolution (provided hardware is compatible). Takes one argument (a resolution name); no arguments show a list of possible resolution names.\n", COL_FOR_BOLD_MAGENTA, COL_FOR_WHITE);
        formatNewLines(shorkresStr, TERM_SIZE.ws_col, "    ");
        printf("%s", shorkresStr);
    }
}

void showCursor(void)
{
    printf("\033[?25h");
    if (COL_ENABLED) printf("\033[%sm", COL_RESET);
}

void showHelp(void)
{
    char desc[120] = "Displays help and reference information for using SHORK 486 and its including software and tools.\n";
    formatNewLines(desc, TERM_SIZE.ws_col, NULL);
    printf("%s\n", desc);

    char usage[60] = "Usage: shorkhelp [OPTIONS]\n\n";
    formatNewLines(usage, TERM_SIZE.ws_col, NULL);
    printf("%s", usage);

    char options[20] = "Options:\n";
    formatNewLines(options, TERM_SIZE.ws_col, NULL);
    printf("%s", options);

    char commands[100] = "--commands         Displays commands & programs list and exit\n";
    formatNewLines(commands, TERM_SIZE.ws_col, "                   ");
    printf("%s", commands);

    if (isProgramInstalled("emacs") || isProgramInstalled("mg"))
    {
        char emacs[100] = "--emacs            Displays Emacs (Mg) cheatsheet and exit\n";
        formatNewLines(emacs, TERM_SIZE.ws_col, "                   ");
        printf("%s", emacs);
    }

    char help[100] = "-h, --help         Displays help information and exits\n";
    formatNewLines(help, TERM_SIZE.ws_col, "                   ");
    printf("%s", help);

    if (isProgramInstalled("git"))
    {
        char git[100] = "--git              Displays supported Git commands and exits\n";
        formatNewLines(git, TERM_SIZE.ws_col, "                   ");
        printf("%s", git);
    }

    char intro[100] = "--intro            Displays introduction to SHORK 486 and exit\n";
    formatNewLines(intro, TERM_SIZE.ws_col, "                   ");
    printf("%s", intro);

    if (isProgramInstalled("sl") || isProgramInstalled("shorksay"))
    {
        char shorktainment[100] = "--shorktainment    Displays SHORK Entertainment list and exit\n";
        formatNewLines(shorktainment, TERM_SIZE.ws_col, "                   ");
        printf("%s", shorktainment);
    }

    char shorkutils[100] = "--shorkutils       Displays SHORK Utilities list and exit\n";
    formatNewLines(shorkutils, TERM_SIZE.ws_col, "                   ");
    printf("%s", shorkutils);
}

void showMainMenu(void)
{
    if (TERM_SIZE.ws_col < 60 || TERM_SIZE.ws_row < 10)
    {
        printf("ERROR: terminal size too small (must be 60x10 or larger)\n");
        return;
    }
    
    setvbuf(stdout, NULL, _IONBF, 0);
    atexit(showCursor);
    atexit(disableRawMode);

    enableRawMode();
    printf("\033[?25l");

    int running = 1;
    int cursor = 1;
    int cursorPrev = 0;
    int fullRedraw = 1;

    MenuItem menu[] = {
        { 
            "intro",
            "Introduction to SHORK 486",
            printIntro,
            1 },
        {
            "commands",
            "Commands & programs list",
            printCommands,
            1 },
        { 
            "shorkutils",
            "SHORK Utilities list",
            printSHORKUtilities,
            1 },
        { 
            "shorktainment",
            "SHORK Entertainment list",
            printSHORKEntertainment,
            isProgramInstalled("shorklocomotive") || isProgramInstalled("shorksay") },
        {
            "emacs",
            "Emacs (Mg) cheatsheet",
            printEmacsCheatsheet,
            isProgramInstalled("emacs") },
        {
            "git",
            "Supported Git commands",
            printGitCommands,
            isProgramInstalled("git") },
    };
    int menuSize = sizeof(menu) / sizeof(menu[0]);
    int indices[menuSize];

    while (running)
    {
        if (fullRedraw)
        {
            clearScreen();
            printHeader("SHORKHELP");
            printMenu(menu, menuSize, cursor, cursorPrev);
            printFooter("[jk] Navigate [Enter] Select [q] Quit");
        }
        else
        {
            if (COL_ENABLED)
                printf("\x1b[2;1H");
            else
                printf("\x1b[3;1H");
            printMenu(menu, menuSize, cursor, cursorPrev);
        }

        enum NavInput input = getNavInput();

        fullRedraw = 1;
        cursorPrev = 0;
        switch (input)
        {
            case CURSOR_UP:
                cursorPrev = cursor;
                cursor--;
                if (cursor < 1) cursor = menuSize;
                fullRedraw = 0;
                break;

            case CURSOR_DOWN:
                cursorPrev = cursor;
                cursor++;
                if (cursor > menuSize) cursor = 1;
                fullRedraw = 0;
                break;

            case ENTER:
                clearScreen();
                menu[cursor - 1].action();
                printf("\033[%d;1H", TERM_SIZE.ws_row);
                awaitInput();
                break;
        
            case QUIT:
                running = 0;
                cursor = 0;
                break;
        }
    }

    clearScreen();
}



int main(int argc, char *argv[])
{
    TERM_SIZE = getTerminalSize();

    if (argc == 1 || argc > 2) showMainMenu();
    else if (argc == 2)
    {
        if (strcmp(argv[1], "--commands") == 0)
        {
            clearScreen();
            printCommands();
        }
        else if (strcmp(argv[1], "--emacs") == 0 || strcmp(argv[1], "--mg") == 0)
        {
            clearScreen();
            printEmacsCheatsheet();
        }
        else if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--hekp") == 0)
        {
            showHelp();
        }
        else if (strcmp(argv[1], "--git") == 0)
        {
            clearScreen();
            printGitCommands();
        }
        else if (strcmp(argv[1], "--intro") == 0)
        {
            clearScreen();
            printIntro();
        }
        else if (strcmp(argv[1], "--shorktainment") == 0)
        {
            clearScreen();
            printSHORKEntertainment();
        }
        else if (strcmp(argv[1], "--shorkutils") == 0)
        {
            clearScreen();
            printSHORKUtilities();
        }
        else showHelp();
    }

    return 0;
}
