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



#include <ctype.h>
#include <sys/ioctl.h>
#include <linux/limits.h>
#include <signal.h>
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

#define CSV_BUFFER              32768
#define MAX_CMD_STR             2048
#define MAX_PROG_ENTRIES        300

static int AVAIL_HEIGHT;
static int BASE_ROW;
static int COL_ENABLED = 1;
static char *COL_FOR_ARROW = COL_FOR_BOLD_RED;
static char *COL_FOR_CODE = COL_FOR_BOLD_RED;
static char *COL_FOR_CURSOR = COL_FOR_BOLD_CYAN;
static char *COL_FOR_HEADING = COL_FOR_BOLD_CYAN;
static char *COL_FOR_OL = COL_FOR_BOLD_GREEN;
static char *COL_FOR_SHORKUTIL = COL_FOR_BOLD_MAGENTA;
static char CURSOR_CHAR = '*';
static struct termios OLD_TERMIOS;
static char OS_NAME[128];
static ProgramEntry PROG_ENTRIES[MAX_PROG_ENTRIES];
static int PROG_ENTRIES_NO = -1;
static struct winsize TERM_SIZE;



/**
 * Awaits for any user input.
 */
void awaitInput(void)
{
    printf("\033[%d;1H", TERM_SIZE.ws_row);
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
 * Extracts a substring from an input string after a given separation character
 * and offset. Also removes any surrounding quotes or trailing newline characters
 * present. 
 * @param input Input string
 * @param point Character to find to separate from (e.g., '=' or ':')
 * @param offset How many characters after the point to separate at
 * @param inputSize Size to use when allocating the result string
 * @return String containing what's left after separation and cleaning
 */
char *extractFromPoint(char *input, size_t inputSize, char point, int offset)
{
    if (!input || inputSize < 2) return strdup("");

    // Prepare result string
    char *result = malloc(inputSize);
    if (!result) return strdup("");
    result[0] = '\0';

    // Find our separation point in the input string
    char *sep = strchr(input, point);
    if (!sep) return result;

    // Our start position taking into account possible offset
    char *start = sep + offset;

    // Trim potential leading double quote
    if (*start == '"') start++;

    // Copy everything after the start position into our result
    strncpy(result, start, inputSize - 1);
    result[inputSize - 1] = '\0';
    size_t len = strlen(result);

    // Trim potential trailing newline 
    if (len > 0 && result[len - 1] == '\n')
        result[--len] = '\0';

    // Trim potential trailing double quote
    if (len > 0 && result[len - 1] == '"')
        result[len - 1] = '\0';

    return result;
}

/**
 * Checks if the given file exists and is accessible.
 * @param file Full path to file
 * @return 1 if file found; 0 if file not found
 */
int fileExists(const char *file)
{
    if (strchr(file, '/')) return access(file, R_OK) == 0;
    else return 0;
}

/**
 * Finds and replaces a given search term with a desired replacement term from an
 * input string.
 * @param input Input string
 * @param inputSize Size to use when allocating the result string
 * @param needle Substring to find and replace
 * @param replacement New string to insert
 * @return String after term replacement
 */
char *findReplace(const char *input, const size_t inputSize, const char *needle, const char *replacement)
{
    if (!input || !needle || !replacement || inputSize < 2) return strdup("");

    size_t needleLen = strlen(needle);
    size_t replacementLen = strlen(replacement);
    if (needleLen == 0) return strdup("");

    // Prepare result string
    char *result = malloc(inputSize);
    if (!result) return strdup("");

    // Copy input string to result
    strncpy(result, input, inputSize);
    result[inputSize - 1] = '\0';

    char *pos = result;
    while ((pos = strstr(pos, needle)) != NULL)
    {
        size_t tailLen = strlen(pos + needleLen);

        // If replacement is larger than our needle, realloc memory to avoid overflowing
        if (replacementLen > needleLen)
        {
            size_t currentLen = strlen(result);
            size_t newLen = currentLen + (replacementLen - needleLen) + 1;
            char *tmp = realloc(result, newLen);
            if (!tmp) break;
            pos = tmp + (pos - result);
            result = tmp;
        }

        // Move the trailing text to accomodate the new size and paste our replacement into
        // the 'gap'
        memmove(pos + replacementLen, pos + needleLen, tailLen + 1);
        memcpy(pos, replacement, replacementLen);
        pos += replacementLen;
    }

    return result;
}

/**
 * Adds new lines to a given string based on the requested line width.
 * @param input Input string
 * @param width Characters per line
 * @param indent Indent to include after newly inserted new line
 * @param trim Flags that any trailing newlines should be removed
 * @return Number of lines in the string
 */
int formatNewLines(char *input, int width, char *indent, int trim)
{
    if (!input || width < 1) return 0;

    // Initialse variables that help us track progress
    size_t inputStrLen = strlen(input);
    size_t indentLen = indent ? strlen(indent) : 0;
    int lines = 1;
    int lastSpace = -1;
    int widthCount = 1;

    // Iterate through the input string to find line breaks or places to add new ones
    for (int i = 0; i < inputStrLen; i++)
    {
        if (input[i] == '\033')
        {
            while (i < inputStrLen && input[i] != 'm') i++;
            if (i >= inputStrLen) break;
            continue; 
        }
        
        // Track where the last space was in case so we can go back for a future word wrap
        if (input[i] == ' ') lastSpace = i;
        // Reset tracking and take into account if we find an existing new line
        else if (input[i] == '\n')
        {
            lines++;
            widthCount = 0;
            continue;
        }

        // Begin word wrapping once the line width is saturated
        if (widthCount == width)
        {
            if (lastSpace != -1)
            {
                input[lastSpace] = '\n';
                lines++;

                if (indent && indentLen > 0)
                {
                    memmove(input + lastSpace + 1 + indentLen, input + lastSpace + 1, inputStrLen - lastSpace);
                    memcpy(input + lastSpace + 1, indent, indentLen);
                    inputStrLen += indentLen;
                    if (lastSpace <= i) i += indentLen;
                }
            }
            widthCount = i - lastSpace;
        }

        widthCount++;
    }

    // If desired, strip possible trailing new line
    if (trim)
    {
        int end = strlen(input) - 1;
        while (end >= 0 && input[end] == '\n')
        {
            input[end] = '\0';
            end--;
            lines--;
        }
    }

    return lines;
}

/** 
 * Returns the full path to the directory this program is stored in.
 * @return Full path the binary is stored in
 */
char *getBinDir(void)
{
    // Get binary's full path
    static char binDir[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", binDir, sizeof(binDir) - 1);
    if (len <= 0) return NULL;
    binDir[len] = '\0';

    // Remove filename from path
    char *slash = strrchr(binDir, '/');
    if (!slash) return NULL;
    *(slash + 1) = '\0';

    return binDir;
}

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
 * Parses a line taken from a CSV list into separate fields.
 * @param line Raw line from CSV file to be processed
 * @param out Array of separated fields
 * @param maxFields Max number of fields to look for
 * @return Number of fields detected
 */
int loadCSVLine(char *line, char *out[], int maxFields)
{
    int i = 0;

    while (*line && i < maxFields)
    {
        out[i++] = line;
        int inQuotes = 0;

        while (*line)
        {
            if (*line == '"')
            {
                inQuotes = !inQuotes;
                if (line[1] == '"') line++;
            }
            else if (*line == ',' && !inQuotes)
            {
                *line = '\0';
                line++;
                break;
            }

            line++;
        }
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

        if (!isOptional || (isOptional && isProgramInstalled(fields[0])))
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

/**
 * Gets how many valid rows there are in the current menu column. Used to help clamp menu cursors.
 */
int rowsInCol(int menuSize, int rows, int col)
{
    int start = (col - 1) * rows;
    int end   = start + rows;
    if (start >= menuSize) return 0;
    if (end > menuSize) return menuSize - start;
    return rows;
}

void showCursor(void)
{
    printf("\033[?25h");
    if (COL_ENABLED) printf("\033[%sm", COL_RESET);
}

/**
 * Splits a given string via any newline escape sequences into an array of strings.
 * @param text Text to split
 * @param textLines Text once split
 * @param totalLines Number of newlines detected
 */
void splitText(char *text, char *textLines[], int totalLines)
{
    int count = 0;
    char *curr = text;
    char *start = text;

    while (*curr && count < totalLines)
    {
        if (*curr == '\n')
        {
            *curr = '\0';
            textLines[count++] = start;
            start = curr + 1;
        }
        curr++;
    }

    if (count < totalLines)
        textLines[count++] = start;
}

/**
 * Makes the terminal cursor visible again, resets the terminal's colours and clears the screen
 * upon exiting.
 */
void onExit(void)
{
    disableRawMode();
    showCursor();
    clearScreen();
}

/**
 * Used to handle exiting controllably if SIGINT is received (Ctrl+C).
 */
void onSigInt(int sig)
{
    exit(0);
}



/**
 * @param footnote Text to be printed in the footer
 */
void printFooter(char *footnote)
{
    printf("\033[%d;1H", TERM_SIZE.ws_row);
    
    if (COL_ENABLED)
        printf("\033[%s;%sm", COL_FOR_BOLD_WHITE, COL_BAK_BLUE);
    else
        for (int i = 0; i < TERM_SIZE.ws_col; i++) printf("-");

    int len = printf("%s", footnote);

    if (COL_ENABLED)
    {
        for (int i = len; i < TERM_SIZE.ws_col; i++) printf(" ");
        printf("\033[%sm", COL_RESET);
    }
}

/**
 * @param title Text to be printed in the header
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

/**
 * @param menu The menu to print
 * @param menuSize How many items are in the menu
 * @param cols Number of columns to draw
 * @param colWidth How many characters are in a column (excluding the cursor indicator)
 * @param rows Number of rows to draw
 * @param cursorX Current column cursor position
 * @param cursorY Current row cursor position
 * @param cursorXPrev Previous column cursor position
 * @param cursorYPrev Previous row cursor position
 */
void printMenu(MenuItem *menu, int menuSize, int cols, int colWidth, int rows, int cursorX, int cursorY, int cursorXPrev, int cursorYPrev)
{
    // Viewport offset and clamping for current row cursor
    int offset = (cursorY - 1) - (AVAIL_HEIGHT / 2);
    if (offset < 0) offset = 0;
    if (offset > rows - AVAIL_HEIGHT) offset = rows - AVAIL_HEIGHT;
    if (offset < 0) offset = 0;

    // Viewport offset and clamping for previous row cursor
    int prevOffset = (cursorYPrev - 1) - (AVAIL_HEIGHT / 2);
    if (prevOffset < 0) prevOffset = 0;
    if (prevOffset > rows - AVAIL_HEIGHT) prevOffset = rows - AVAIL_HEIGHT;
    if (prevOffset < 0) prevOffset = 0;



    int inScrolling = (prevOffset != offset);

    // cursorPrev is initialised as 0 to indicate first frame should be drawn
    if (cursorYPrev == 0) inScrolling = 1;

    int prevIndex = cursorYPrev - 1;
    int currIndex = cursorY - 1;

    // If we don't need to scroll, just update the cursor
    if (!inScrolling)
    {
        // Remove old line cursor
        int prevCol = 1 + (cursorXPrev - 1) * (colWidth + 3);
        int prevRow = BASE_ROW + (cursorYPrev - 1 - offset);
        printf("\x1b[%d;%dH   ", prevRow, prevCol);

        // Print new line cursor
        int currCol = 1 + (cursorX - 1) * (colWidth + 3);
        int currRow = BASE_ROW + (cursorY - 1 - offset);
        printf("\x1b[%d;%dH \033[%sm%c\033[%sm ", currRow, currCol, COL_FOR_CURSOR, CURSOR_CHAR, COL_RESET);
        
        return;
    }



    int canGoUp = offset > 0;
    int canGoDown = (offset + AVAIL_HEIGHT) < rows;
    int linesPrinted = 0;

    for (int i = offset; i < rows && linesPrinted < AVAIL_HEIGHT; i++)
    {
        printf("\x1b[%d;1H\x1b[K", BASE_ROW + linesPrinted);

        // Can scroll up indicator
        if (canGoUp && i == offset)
            printf("\033[%sm^\033[%sm\x1b[K\n", COL_FOR_ARROW, COL_RESET);
        // Can scroll down indicator
        else if (canGoDown && i == offset + AVAIL_HEIGHT - 1)
            printf("\033[%smv\033[%sm\n", COL_FOR_ARROW, COL_RESET);
        // Selected row
        else
        {
            for (int j = 0; j < cols; j++)
            {
                int cursor = i + j * rows;
                if (cursor < menuSize)
                {
                    if (j + 1 == cursorX && i + 1 == cursorY)
                        printf(" \033[%sm%c\033[%sm %-*s", COL_FOR_CURSOR, CURSOR_CHAR, COL_RESET, colWidth, menu[cursor].name);
                    else
                        printf("   %-*s", colWidth, menu[cursor].name);
                }
            }
            printf("\n");
        }

        linesPrinted++;
    }



    // "Fill in" remaining viewport lines
    for (int i = linesPrinted; i < AVAIL_HEIGHT; i++)
        printf("\n");
}

/**
 * @param text Text for the main body
 * @param totalLines How many newlines are present in main body text
 * @param pageScroll Flags if scrolling should be page based instead of line-by-line
 */
void printScrollingText(char *text, int totalLines, int pageScroll)
{
    if (totalLines < AVAIL_HEIGHT)
    {
        printf("\x1b[%d;1H\x1b[K", BASE_ROW);
        printf("%s", text);
        awaitInput();
        return;
    }
    else printFooter("[jk] Navigate [q] Back");

    char *textLines[totalLines];
    if (totalLines > 1) splitText(text, textLines, totalLines);
    else textLines[0] = text;

    int offscreen = totalLines - AVAIL_HEIGHT;

    int running = 1;
    int cursor = 0;
    int canGoUp = 0;
    int canGoDown = 0;

    while (running)
    {
        canGoUp = cursor > 0;
        canGoDown = (cursor + AVAIL_HEIGHT) < totalLines;

        for (int i = 0; i < AVAIL_HEIGHT; i++)
        {
            printf("\x1b[%d;1H\x1b[K", BASE_ROW + i);

            if (canGoUp && i == 0)
                printf("\033[%sm^\033[%sm\x1b[K\n", COL_FOR_ARROW, COL_RESET);
            else if (canGoDown && i == AVAIL_HEIGHT - 1)
                printf("\033[%smv\033[%sm\x1b[K\n", COL_FOR_ARROW, COL_RESET);
            else
                printf("%s", textLines[i + cursor]);
        }

        enum NavInput input = getNavInput();
        switch (input)
        {
            case CURSOR_UP:
                if (pageScroll) cursor -= (AVAIL_HEIGHT - 3);
                else cursor--;
                if (cursor < 0) cursor = 0;
                break;

            case CURSOR_DOWN:
                if (pageScroll) cursor += (AVAIL_HEIGHT - 3);
                else cursor++;
                if (cursor > offscreen) cursor = offscreen;
                break;

            case QUIT:
                running = 0;
                break;
        }
    }
}

void setupForMenu(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    atexit(onExit);
    signal(SIGINT, onSigInt);

    enableRawMode();
    printf("\033[?25l");
}



void printCommands(void)
{
    printHeader("Commands & programs list");

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
    printScrollingText(combinedStr, lines, 1);
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
    printScrollingText(emacsStr, lines, 1);
}

void printGitCommands(void)
{
    printHeader("Supported Git commands");

    char gitStr[500];
    snprintf(gitStr, 500, "add, blame, branch, checkout, cherry-pick, clean, clone, commit, config, diff, fetch, init, log, merge, mv, pull, push, rebase, reflog, remote, reset, restore, rm, show, stash, status, switch, tag\n");

    int lines = formatNewLines(gitStr, TERM_SIZE.ws_col, NULL, 1);
    printScrollingText(gitStr, lines, 1);
}

void printIntro(void)
{
    printHeader("Introduction to SHORK 486");

    const int strSize = 3072;
    char introStr[strSize];
    int pos = 0;

    pos += snprintf(introStr + pos, strSize - pos, "SHORK 486 is a 32-bit Linux distribution for 486 and Pentium (P5) PCs! It focuses on being as lean and small as possible, whilst still providing a modern kernel, robust command set, custom utilities, and hand-picked modern software.\n\n");

    pos += snprintf(introStr + pos, strSize - pos, "\033[%smGoals\033[%sm\nBesides being something fun to try on old PCs, SHORK 486 was founded on the belief that old PCs can still be useful to the right people, retrocomputing and gaming usage aside. SHORK 486 can be useful for lightweight desktop usage, SSH terminal usage, distraction-free typewriting (\"writerdecks\"), embedded applications, demonstrative use in academia/education, and as a technical demonstration of what old PCs and modern software targeted at them can actually still do! If it can save just one more PC from the landfills, it has done its job!\n\n", COL_FOR_HEADING, COL_FOR_WHITE);

    pos += snprintf(introStr + pos, strSize - pos, "\033[%smWhat's special\033[%sm\nSHORK 486 is a modern and maintained Linux distribution that can run on a processor architecture from 1989. Depending on configuration, it only requires between 8 and 24MiB system memory whilst still packing a lot of functionality for its size. Due to various factors, making such a distribution is increasingly difficult in the 2020s. System requirements are ever-increasing, with even the otherwise excellent Micro Core and Tiny Core requiring at least 26-46MB RAM, putting them out of range of many early 486 systems. As of Linux kernel 7.1 and beyond, support for 486 processors and various ISA and PCMCIA networking hardware has been dropped, and 32-bit x86 support in general is currently dropped by most mainstream distributions. Given this situation, SHORK 486 will try to fill this niche of a ready-to-go Linux distribution for such PCs by sticking with a minimal-where-possible philosophy, customisability and restoring support for older hardware with newer Linux kernels!\n\n", COL_FOR_HEADING, COL_FOR_WHITE);

    pos += snprintf(introStr + pos, strSize - pos, "\033[%smArchitecture\033[%sm\nSHORK 486 is not GNU/Linux as you may be accustomed to. Its init system and most core utilities are provided by BusyBox, a single-binary application well known for embedded usage. As needed, some util-linux and individual utilities are used to fill holes in BusyBox\'s suite. The system is compiled with musl instead of glibc, producing smaller binaries that also use fewer resources. The closest well-known Linux distribution to SHORK 486 is Alpine Linux.\n\n", COL_FOR_HEADING, COL_FOR_WHITE);

    pos += snprintf(introStr + pos, strSize - pos, "\033[%smOrigin & inspirations\033[%sm\nIn December 2025, YouTuber Action Retro posted a video on FLOPPINUX, something that turned out to be a very accessible means for me to learn how to make a working Linux system. It foremost inspired me to chase my dream of building a viable modern Linux system for my old IBM ThinkPads. SHORK Mini (as it was originally known) began as an automated build script based on FLOPPINUX\'s build instructions, but adapted for producing fixed disk images instead of diskette images. After that, more Linux kernel and BusyBox features were enabled, and other software was compiled to help fulfil that dream. Other inspirations from similar efforts include Gray386linux and Ocawesome101\'s blog post on running Linux on a 486SX. Aspects of Alpine Linux and Tiny Core are also kept in mind.", COL_FOR_HEADING, COL_FOR_WHITE);

    int lines = formatNewLines(introStr, TERM_SIZE.ws_col, NULL, 0);
    printScrollingText(introStr, lines, 1);
}

void printProgOverview(int i)
{
    // Header
    printHeader(PROG_ENTRIES[i].command);

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
    printScrollingText(overviewStr, lines, 1);
}

void printPT1(void)
{
    printHeader("Public Test 1");

    const int strSize = 3072;
    char pt1Str[strSize];
    int pos = 0;

    pos += snprintf(pt1Str + pos, strSize - pos, "You are running a pre-release build of SHORK 486 - Public Test 1 (PT1). This is the first published release of SHORK 486, intended for testing purposes only and to gauge wider opinion to help the project's progression.\n\n");

    pos += snprintf(pt1Str + pos, strSize - pos, "\033[%smPurpose\033[%sm\nSHORK 486's development has progressed far enough that it has achieved the author's initial objectives, and is now appropriate for the first round of public testing to help scope out what to tackle next. PT1 also coincides with Linux kernel 7.1's debut, the first version to remove 486 and other relevant vintage hardware support, which this project subsequently patches back in. It would be ideal to validate that these patches are working correctly.\n\n", COL_FOR_HEADING, COL_FOR_WHITE);

    pos += snprintf(pt1Str + pos, strSize - pos, "\033[%smLicence & disclaimers\033[%sm\nSHORK 486 is a non-commercial, free and open-source operating system licenced under GPLv3 that is provided \"as is\" without warranty of any kind. In no event shall the contributors be liable for any damages arising from use of this software. Additionally, SHORK 486 is a work in progress, and this is a pre-release build. It should not be used for serious, production, or mission-critical work.\n\n", COL_FOR_HEADING, COL_FOR_WHITE);

    pos += snprintf(pt1Str + pos, strSize - pos, "\033[%smSuggestions & feedback\033[%sm\nIdeas for what to add and change to SHORK 486 are welcome! These can be suggestions for new bundled programs, new or improved features, and advice on how the system is configured and built. Frank and critical feedback on anything regarding the project is also welcomed, though it is expected that it is respectful and understanding. However, please understand that otherwise well-intentioned and interesting suggestions may be considered beyond the scope of this project, as the aim is to keep the system as minimal as possible, but they will be remembered for future sister projects. Suggestions and feedback can be given on the SHORK 486 GitHub repository or via social media channels found on \033[%smlinks.sharktastica.co.uk\033[%sm.\n\n", COL_FOR_HEADING, COL_FOR_WHITE, COL_FOR_CODE, COL_FOR_WHITE);

    pos += snprintf(pt1Str + pos, strSize - pos, "\033[%smBug & error reporting\033[%sm\nSHORK 486 does not collect any telemetry nor report anything to anyone, instead relying on you, the tester, to report and provide evidence of any issues that may arise when using it. If you encounter any, please report them! It is a good measure to include a robust description of your hardware and what you were trying to do, whether you compiled it yourself or used published install media, a screenshot/photo of \033[%smshorkfetch\033[%sm's output, \033[%sm/proc/cpuinfo\033[%sm's contents, and \033[%smdmesg\033[%sm output. Reports can be given on the SHORK 486 GitHub repository.", COL_FOR_HEADING, COL_FOR_WHITE, COL_FOR_CODE, COL_FOR_WHITE, COL_FOR_CODE, COL_FOR_WHITE, COL_FOR_CODE, COL_FOR_WHITE);

    int lines = formatNewLines(pt1Str, TERM_SIZE.ws_col, NULL, 0);
    printScrollingText(pt1Str, lines, 1);
}

void printSHORKEntertainment(void)
{
    printHeader("SHORK Entertainment");

    const int strSize = 750;
    char shorktainmentStr[strSize];
    int pos = 0;

    if (isProgramInstalled("sl"))
        pos += snprintf(shorktainmentStr + pos, strSize - pos, "\033[%smshorklocomotive\033[%sm: A shark-themed take on the \033[%smsl\033[%sm command that kindly pokes fun at making typos when trying to type \033[%smls\033[%sm.\n", COL_FOR_SHORKUTIL, COL_FOR_WHITE, COL_FOR_CODE, COL_FOR_WHITE, COL_FOR_CODE, COL_FOR_WHITE);

    if (isProgramInstalled("shorkmatrix"))
        pos += snprintf(shorktainmentStr + pos, strSize - pos, "\033[%smshorkmatrix\033[%sm: A quick, blue-themed take on the \033[%smcmatrix\033[%sm \"digital rain\" vertical scrolling text screensaver.\n", COL_FOR_SHORKUTIL, COL_FOR_WHITE, COL_FOR_CODE, COL_FOR_WHITE);

    if (isProgramInstalled("shorksay"))
        pos += snprintf(shorktainmentStr + pos, strSize - pos, "\033[%smshorksay\033[%sm: A shark-themed take on the \033[%smcowsay\033[%sm command that outputs an ASCII art shark and speech bubble containing a message of your choice.\n", COL_FOR_SHORKUTIL, COL_FOR_WHITE, COL_FOR_CODE, COL_FOR_WHITE);

    int lines = formatNewLines(shorktainmentStr, TERM_SIZE.ws_col, "    ", 1);
    printScrollingText(shorktainmentStr, lines, 1);
}

void printSHORKUtilities(void)
{
    printHeader("SHORK Utilities");

    const int strSize = 2000;
    char shorkutilStr[strSize];
    int pos = 0;

    if (isProgramInstalled("shorkdir"))
        pos += snprintf(shorkutilStr + pos, strSize - pos, "\033[%smshorkdir\033[%sm: Terminal-based file browser and file inspector (if \033[%smfile\033[%sm is installed).\n", COL_FOR_SHORKUTIL, COL_FOR_WHITE, COL_FOR_CODE, COL_FOR_WHITE);

    if (isProgramInstalled("shorkfetch"))
        pos += snprintf(shorkutilStr + pos, strSize - pos, "\033[%smshorkfetch\033[%sm: Displays basic system and environment information. Similar to \033[%smfastfetch\033[%sm, \033[%smneofetch\033[%sm, etc.\n", COL_FOR_SHORKUTIL, COL_FOR_WHITE, COL_FOR_CODE, COL_FOR_WHITE, COL_FOR_CODE, COL_FOR_WHITE);

    if (isProgramInstalled("shorkfont"))
        pos += snprintf(shorkutilStr + pos, strSize - pos, "\033[%smshorkfont\033[%sm: Changes the terminal's font or colour. Takes two arguments (type of change and name); no arguments shows how to use and a list of possible colours.\n", COL_FOR_SHORKUTIL, COL_FOR_WHITE);

    pos += snprintf(shorkutilStr + pos, strSize - pos, "\033[%smshorkhelp\033[%sm: Provides help with using SHORK 486 via command lists, guides and cheatsheets. Requires the use of one of five parameters.\n", COL_FOR_SHORKUTIL, COL_FOR_WHITE);

    if (isProgramInstalled("shorkmap"))
        pos += snprintf(shorkutilStr + pos, strSize - pos, "\033[%smshorkmap\033[%sm: Changes the system's keyboard map. Takes takes one argument (a keymap name); no arguments show a list of possible keymaps.\n", COL_FOR_SHORKUTIL, COL_FOR_WHITE);

    if (isProgramInstalled("shorkoff"))
        pos += snprintf(shorkutilStr + pos, strSize - pos, "\033[%smshorkoff\033[%sm: Brings the system to a halt and syncs the write cache, allowing the computer to be safely turned off. Similar to \033[%smpoweroff\033[%sm or \033[%smshutdown -h\033[%sm.\n", COL_FOR_SHORKUTIL, COL_FOR_WHITE, COL_FOR_CODE, COL_FOR_WHITE, COL_FOR_CODE, COL_FOR_WHITE);

    if (isProgramInstalled("shorkres"))
        pos += snprintf(shorkutilStr + pos, strSize - pos, "\033[%smshorkres\033[%sm: Changes the system's display resolution (provided hardware is compatible). Takes one argument (a resolution name); no arguments show a list of possible resolution names.\n", COL_FOR_SHORKUTIL, COL_FOR_WHITE);

    int lines = formatNewLines(shorkutilStr, TERM_SIZE.ws_col, "    ", 1);
    printScrollingText(shorkutilStr, lines, 1);
}

void printStarted(void)
{
    printHeader("Getting started");

    char gettingStartedStr[1200];
    snprintf(gettingStartedStr, 1200, 
"\033[%sm1.\033[%sm Set your keyboard's layout with \033[%smshorkmap\033[%sm.\n\
\033[%sm2.\033[%sm Pick a font and colour for the console terminal with \033[%smshorkfont\033[%sm.\n\
\033[%sm3.\033[%sm (If compatible) Change your display's resolution with \033[%smshorkres\033[%sm. A reboot will be required.\n\
\033[%sm4.\033[%sm Set your computer's name by editing \033[%sm/etc/hostname\033[%sm. A reboot will be required, or you can run: \033[%smhostname \"$(cat /etc/hostname)\"\n\
\033[%sm5.\033[%sm (If applicable) Test your network connection with \033[%smping\033[%sm. For example: \033[%smping sharktastica.co.uk\n\
\033[%sm6.\033[%sm Run \033[%smshorkfetch\033[%sm to see a quick overview of your system and environment.\n\
\033[%sm7.\033[%sm Check \033[%smshorkhelp\033[%sm's other options to learn what commands and software are available, and see what guides may be of use.\n\
\033[%sm8.\033[%sm Use Ctrl+Alt+F1/F2/F3 or \033[%smchvt\033[%sm to switch between the three available virtual consoles, useful for multitasking, troubleshooting and recovery.\n\
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
    printScrollingText(gettingStartedStr, lines, 1);
}

void printTmuxCheatsheet(void)
{
    printHeader("tmux cheatsheet");
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
    printScrollingText(tmuxStr, lines, 1);
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
        menu[i].id = menu[i].name = PROG_ENTRIES[i].command;
        menu[i].action = NULL;
        menu[i].visible = 1;
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
            printMenu(menu, PROG_ENTRIES_NO, cols, colWidth, rows, cursorX, cursorY, cursorXPrev, cursorYPrev);
            printFooter("[hjkl] Navigate [Enter] Select [q] Back");
        }
        else
        {
            if (COL_ENABLED)
                printf("\x1b[2;1H");
            else
                printf("\x1b[3;1H");
            printMenu(menu, PROG_ENTRIES_NO, cols, colWidth, rows, cursorX, cursorY, cursorXPrev, cursorYPrev);
        }

        enum NavInput input = getNavInput();

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

    if (isProgramInstalled("emacs") || isProgramInstalled("mg"))
    {
        char emacs[100] = "--emacs          Displays Emacs (Mg) cheatsheet and exit\n";
        formatNewLines(emacs, TERM_SIZE.ws_col, "                 ", 0);
        printf("%s", emacs);
    }

    char help[100] = "-h, --help       Displays help information and exits\n";
    formatNewLines(help, TERM_SIZE.ws_col, "                 ", 0);
    printf("%s", help);

    if (isProgramInstalled("git"))
    {
        char git[100] = "--git            Displays supported Git commands and exits\n";
        formatNewLines(git, TERM_SIZE.ws_col, "                 ", 0);
        printf("%s", git);
    }

    char intro[100] = "--intro          Displays introduction to SHORK 486 and exit\n";
    formatNewLines(intro, TERM_SIZE.ws_col, "                 ", 0);
    printf("%s", intro);

    if (getIsPT1())
    {
        char pt1[80] = "--pt1            Displays Public Test 1 information and exits\n";
        formatNewLines(pt1, TERM_SIZE.ws_col, "                 ", 0);
        printf("%s", pt1);
    }

    if (isProgramInstalled("sl") || isProgramInstalled("shorkmatrix") || isProgramInstalled("shorksay"))
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

    if (isProgramInstalled("tmux"))
    {
        char tmux[100] = "--tmux           Displays tmux cheatsheet and exit\n";
        formatNewLines(tmux, TERM_SIZE.ws_col, "                 ", 0);
        printf("%s", tmux);
    }
}

/**
 * Runs the main menu interface.
 */
void showMainMenu(void)
{
    if (TERM_SIZE.ws_col < 40 || TERM_SIZE.ws_row < 10)
    {
        printf("ERROR: terminal size too small (must be 40x10 or larger)\n");
        return;
    }

    PROG_ENTRIES_NO = loadProgramEntries();
    if (PROG_ENTRIES_NO == -1)
    {
        printf("ERROR: could not load programs.csv\n");
        return;
    }

    MenuItem rawMenu[] = {
        { 
            "intro",
            "Introduction to SHORK 486",
            printIntro,
            1
        },
        { 
            "pt1",
            "Public Test 1",
            printPT1,
            getIsPT1()
        },
        { 
            "started",
            "Getting started",
            printStarted,
            strncmp(OS_NAME, "SHORK 486", 9) == 0
        },
        {
            "cmdRef",
            "Command reference (WIP)",
            showCommandReference,
            1
        },
        {
            "cmdList",
            "Commands & programs list",
            printCommands,
            1
        },
        { 
            "shorkutils",
            "SHORK Utilities list",
            printSHORKUtilities,
            1
        },
        { 
            "shorktainment",
            "SHORK Entertainment list",
            printSHORKEntertainment,
            isProgramInstalled("shorklocomotive") || isProgramInstalled("shorksay")
        },
        {
            "emacs",
            "Emacs (Mg) cheatsheet",
            printEmacsCheatsheet,
            isProgramInstalled("emacs")
        },
        {
            "git",
            "Supported Git commands",
            printGitCommands,
            isProgramInstalled("git")
        },
        {
            "tmux",
            "tmux cheatsheet",
            printTmuxCheatsheet,
            isProgramInstalled("tmux")
        }
    };
    int rawMenuSize = sizeof(rawMenu) / sizeof(rawMenu[0]);

    int running = 1;
    int cursor = 1;
    int cursorPrev = 0;
    int fullRedraw = 1;

    // Filter menu to just what should actually be visible
    MenuItem menu[rawMenuSize];
    int menuSize = 0;
    for (int i = 0; i < rawMenuSize; i++)
        if (rawMenu[i].visible)
            menu[menuSize++] = rawMenu[i];

    while (running)
    {
        if (fullRedraw)
        {
            clearScreen();
            printHeader("SHORKHELP");
            printMenu(menu, menuSize, 1, TERM_SIZE.ws_col - 6, menuSize, 1, cursor, 1, cursorPrev);
            printFooter("[jk] Navigate [Enter] Select [q] Quit");
        }
        else
        {
            if (COL_ENABLED)
                printf("\x1b[2;1H");
            else
                printf("\x1b[3;1H");
            printMenu(menu, menuSize, 1, TERM_SIZE.ws_col - 6, menuSize, 1, cursor, 1, cursorPrev);
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



int main(int argc, char *argv[])
{
    TERM_SIZE = getTerminalSize();
    if (!getOSName())
    {
        printf("ERROR: Could not retrieve Linux distribution's name\n");
        return 1;
    }

    BASE_ROW = 2;
    AVAIL_HEIGHT = TERM_SIZE.ws_row - 2;
    if (!COL_ENABLED)
    {
        BASE_ROW = 3;
        AVAIL_HEIGHT = TERM_SIZE.ws_row - 4;
    }

    if (argc == 1 || argc > 2)
    {
        setupForMenu();
        showMainMenu();
    }
    else if (argc == 2)
    {
        if (strcmp(argv[1], "--commands") == 0)
        {
            setupForMenu();
            PROG_ENTRIES_NO = loadProgramEntries();
            clearScreen();
            printCommands();
            clearScreen();
        }
        else if (strcmp(argv[1], "--emacs") == 0 || strcmp(argv[1], "--mg") == 0)
        {
            setupForMenu();
            clearScreen();
            printEmacsCheatsheet();
            clearScreen();
        }
        else if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)
            showHelp();
        else if (strcmp(argv[1], "--git") == 0)
        {
            setupForMenu();
            clearScreen();
            printGitCommands();
            clearScreen();
        }
        else if (strcmp(argv[1], "--intro") == 0 && strncmp(OS_NAME, "SHORK 486", 9) == 0)
        {
            setupForMenu();
            clearScreen();
            printIntro();
            clearScreen();
        }
        else if (strcmp(argv[1], "--pt1") == 0 && getIsPT1())
        {
            setupForMenu();
            clearScreen();
            printPT1();
            clearScreen();
        }
        else if (strcmp(argv[1], "--shorktainment") == 0)
        {
            setupForMenu();
            clearScreen();
            printSHORKEntertainment();
            clearScreen();
        }
        else if (strcmp(argv[1], "--shorkutils") == 0)
        {
            clearScreen();
            printSHORKUtilities();
            clearScreen();
        }
        else if (strcmp(argv[1], "--started") == 0)
        {
            setupForMenu();
            clearScreen();
            printStarted();
            clearScreen();
        }
        else if (strcmp(argv[1], "--tmux") == 0)
        {
            setupForMenu();
            clearScreen();
            printTmuxCheatsheet();
            clearScreen();
        }
        else
            showHelp();
    }

    return 0;
}
