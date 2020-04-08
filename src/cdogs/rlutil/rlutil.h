#pragma once
/**
 * File: rlutil.h
 *
 * About: Description
 * This file provides some useful utilities for console mode
 * roguelike game development with C and C++. It is aimed to
 * be cross-platform (at least Windows and Linux).
 *
 * About: Copyright
 * (C) 2010 Tapio Vierros
 *
 * About: Licensing
 * See <License>
 */

/// Define: RLUTIL_USE_ANSI
/// Define this to use ANSI escape sequences also on Windows
/// (defaults to using WinAPI instead).
/// Define: RLUTIL_STRING_T
/// Define/typedef this to your preference to override rlutil's string type.
///
/// Defaults to std::string with C++ and char* with C.
/// Common C++ headers
#include <iostream>
#include <cstring>
#include <string>
#include <cstdio> // for getch()
/// Namespace forward declarations
static void locate(int x, int y);

#include <termios.h> // for getch() and kbhit()
#include <unistd.h> // for getch(), kbhit() and (u)sleep()
#include <sys/ioctl.h> // for getkey()
#include <sys/types.h> // for kbhit()
#include <sys/time.h> // for kbhit()

/// Function: getch
/// Get character without waiting for Return to be pressed.
/// Windows has this in conio.h
static int getch(void) {
	// Here be magic.
	struct termios oldt, newt;
	int ch;
	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;
	newt.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &newt);
	ch = getchar();
	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
	return ch;
}

/// Function: kbhit
/// Determines if keyboard has been hit.
/// Windows has this in conio.h
static int kbhit(void) {
	// Here be dragons.
	static struct termios oldt, newt;
	int cnt = 0;
	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;
	newt.c_lflag &= ~(ICANON | ECHO);
	newt.c_iflag = 0; // input mode
	newt.c_oflag = 0; // output mode
	newt.c_cc[VMIN] = 1; // minimum time to wait
	newt.c_cc[VTIME] = 1; // minimum characters to wait for
	tcsetattr(STDIN_FILENO, TCSANOW, &newt);
	ioctl(0, FIONREAD, &cnt); // Read count
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 100;
	select(STDIN_FILENO + 1, NULL, NULL, NULL, &tv); // A small time delay
	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
	return cnt; // Return number of characters
}

static void gotoxy(int x, int y) {
	locate(x, y);
}

/**
 * Defs: Internal typedefs and macros
 * RLUTIL_STRING_T - String type depending on which one of C or C++ is used
 * RLUTIL_PRINT(str) - Printing macro independent of C/C++
 */
#ifndef RLUTIL_STRING_T
typedef const char *RLUTIL_STRING_T;
#endif // RLUTIL_STRING_T

#define RLUTIL_FPRINT(f, st) fprintf(f, "%s", st)
//#define RLUTIL_PRINT(st) printf("%s", st)
#define RLUTIL_PRINT(st) do { std::cout << st; } while(false)

/**
 * Enums: Color codes
 *
 * BLACK - Black
 * BLUE - Blue
 * GREEN - Green
 * CYAN - Cyan
 * RED - Red
 * MAGENTA - Magenta / purple
 * BROWN - Brown / dark yellow
 * GREY - Grey / dark white
 * DARKGREY - Dark grey / light black
 * LIGHTBLUE - Light blue
 * LIGHTGREEN - Light green
 * LIGHTCYAN - Light cyan
 * LIGHTRED - Light red
 * LIGHTMAGENTA - Light magenta / light purple
 * YELLOW - Yellow (bright)
 * WHITE - White (bright)
 */
enum {
	BLACK,
	BLUE,
	GREEN,
	CYAN,
	RED,
	MAGENTA,
	BROWN,
	GREY,
	DARKGREY,
	LIGHTBLUE,
	LIGHTGREEN,
	LIGHTCYAN,
	LIGHTRED,
	LIGHTMAGENTA,
	YELLOW,
	WHITE
};

/**
 * Consts: ANSI escape strings
 *
 * ANSI_CLS                - Clears screen
 * ANSI_CONSOLE_TITLE_PRE  - Prefix for changing the window title, print the window title in between
 * ANSI_CONSOLE_TITLE_POST - Suffix for changing the window title, print the window title in between
 * ANSI_ATTRIBUTE_RESET    - Resets all attributes
 * ANSI_CURSOR_HIDE        - Hides the cursor
 * ANSI_CURSOR_SHOW        - Shows the cursor
 * ANSI_CURSOR_HOME        - Moves the cursor home (0,0)
 * ANSI_BLACK              - Black
 * ANSI_RED                - Red
 * ANSI_GREEN              - Green
 * ANSI_BROWN              - Brown / dark yellow
 * ANSI_BLUE               - Blue
 * ANSI_MAGENTA            - Magenta / purple
 * ANSI_CYAN               - Cyan
 * ANSI_GREY               - Grey / dark white
 * ANSI_DARKGREY           - Dark grey / light black
 * ANSI_LIGHTRED           - Light red
 * ANSI_LIGHTGREEN         - Light green
 * ANSI_YELLOW             - Yellow (bright)
 * ANSI_LIGHTBLUE          - Light blue
 * ANSI_LIGHTMAGENTA       - Light magenta / light purple
 * ANSI_LIGHTCYAN          - Light cyan
 * ANSI_WHITE              - White (bright)
 * ANSI_BACKGROUND_BLACK   - Black background
 * ANSI_BACKGROUND_RED     - Red background
 * ANSI_BACKGROUND_GREEN   - Green background
 * ANSI_BACKGROUND_YELLOW  - Yellow background
 * ANSI_BACKGROUND_BLUE    - Blue background
 * ANSI_BACKGROUND_MAGENTA - Magenta / purple background
 * ANSI_BACKGROUND_CYAN    - Cyan background
 * ANSI_BACKGROUND_WHITE   - White background
 */
static const RLUTIL_STRING_T ANSI_CLS = "\033[2J\033[3J";
static const RLUTIL_STRING_T ANSI_CONSOLE_TITLE_PRE = "\033]0;";
static const RLUTIL_STRING_T ANSI_CONSOLE_TITLE_POST = "\007";
static const RLUTIL_STRING_T ANSI_ATTRIBUTE_RESET = "\033[0m";
static const RLUTIL_STRING_T ANSI_CURSOR_HIDE = "\033[?25l";
static const RLUTIL_STRING_T ANSI_CURSOR_SHOW = "\033[?25h";
static const RLUTIL_STRING_T ANSI_CURSOR_HOME = "\033[H";
static const RLUTIL_STRING_T ANSI_BLACK = "\033[22;30m";
static const RLUTIL_STRING_T ANSI_RED = "\033[22;31m";
static const RLUTIL_STRING_T ANSI_GREEN = "\033[22;32m";
static const RLUTIL_STRING_T ANSI_BROWN = "\033[22;33m";
static const RLUTIL_STRING_T ANSI_BLUE = "\033[22;34m";
static const RLUTIL_STRING_T ANSI_MAGENTA = "\033[22;35m";
static const RLUTIL_STRING_T ANSI_CYAN = "\033[22;36m";
static const RLUTIL_STRING_T ANSI_GREY = "\033[22;37m";
static const RLUTIL_STRING_T ANSI_DARKGREY = "\033[01;30m";
static const RLUTIL_STRING_T ANSI_LIGHTRED = "\033[01;31m";
static const RLUTIL_STRING_T ANSI_LIGHTGREEN = "\033[01;32m";
static const RLUTIL_STRING_T ANSI_YELLOW = "\033[01;33m";
static const RLUTIL_STRING_T ANSI_LIGHTBLUE = "\033[01;34m";
static const RLUTIL_STRING_T ANSI_LIGHTMAGENTA = "\033[01;35m";
static const RLUTIL_STRING_T ANSI_LIGHTCYAN = "\033[01;36m";
static const RLUTIL_STRING_T ANSI_WHITE = "\033[01;37m";
static const RLUTIL_STRING_T ANSI_BACKGROUND_BLACK = "\033[40m";
static const RLUTIL_STRING_T ANSI_BACKGROUND_RED = "\033[41m";
static const RLUTIL_STRING_T ANSI_BACKGROUND_GREEN = "\033[42m";
static const RLUTIL_STRING_T ANSI_BACKGROUND_YELLOW = "\033[43m";
static const RLUTIL_STRING_T ANSI_BACKGROUND_BLUE = "\033[44m";
static const RLUTIL_STRING_T ANSI_BACKGROUND_MAGENTA = "\033[45m";
static const RLUTIL_STRING_T ANSI_BACKGROUND_CYAN = "\033[46m";
static const RLUTIL_STRING_T ANSI_BACKGROUND_WHITE = "\033[47m";
// Remaining colors not supported as background colors

/**
 * Enums: Key codes for keyhit()
 *
 * KEY_ESCAPE  - Escape
 * KEY_ENTER   - Enter
 * KEY_SPACE   - Space
 * KEY_INSERT  - Insert
 * KEY_HOME    - Home
 * KEY_END     - End
 * KEY_DELETE  - Delete
 * KEY_PGUP    - PageUp
 * KEY_PGDOWN  - PageDown
 * KEY_UP      - Up arrow
 * KEY_DOWN    - Down arrow
 * KEY_LEFT    - Left arrow
 * KEY_RIGHT   - Right arrow
 * KEY_F1      - F1
 * KEY_F2      - F2
 * KEY_F3      - F3
 * KEY_F4      - F4
 * KEY_F5      - F5
 * KEY_F6      - F6
 * KEY_F7      - F7
 * KEY_F8      - F8
 * KEY_F9      - F9
 * KEY_F10     - F10
 * KEY_F11     - F11
 * KEY_F12     - F12
 * KEY_NUMDEL  - Numpad del
 * KEY_NUMPAD0 - Numpad 0
 * KEY_NUMPAD1 - Numpad 1
 * KEY_NUMPAD2 - Numpad 2
 * KEY_NUMPAD3 - Numpad 3
 * KEY_NUMPAD4 - Numpad 4
 * KEY_NUMPAD5 - Numpad 5
 * KEY_NUMPAD6 - Numpad 6
 * KEY_NUMPAD7 - Numpad 7
 * KEY_NUMPAD8 - Numpad 8
 * KEY_NUMPAD9 - Numpad 9
 */
enum {
	KEY_ESCAPE = 0,
	KEY_ENTER = 1,
	KEY_SPACE = 32,

	KEY_INSERT = 2,
	KEY_HOME = 3,
	KEY_PGUP = 4,
	KEY_DELETE = 5,
	KEY_END = 6,
	KEY_PGDOWN = 7,

	KEY_UP = 14,
	KEY_DOWN = 15,
	KEY_LEFT = 16,
	KEY_RIGHT = 17,

	KEY_F1 = 18,
	KEY_F2 = 19,
	KEY_F3 = 20,
	KEY_F4 = 21,
	KEY_F5 = 22,
	KEY_F6 = 23,
	KEY_F7 = 24,
	KEY_F8 = 25,
	KEY_F9 = 26,
	KEY_F10 = 27,
	KEY_F11 = 28,
	KEY_F12 = 29,

	KEY_NUMDEL = 30,
	KEY_NUMPAD0 = 31,
	KEY_NUMPAD1 = 127,
	KEY_NUMPAD2 = 128,
	KEY_NUMPAD3 = 129,
	KEY_NUMPAD4 = 130,
	KEY_NUMPAD5 = 131,
	KEY_NUMPAD6 = 132,
	KEY_NUMPAD7 = 133,
	KEY_NUMPAD8 = 134,
	KEY_NUMPAD9 = 135
};

/// Function: getkey
/// Reads a key press (blocking) and returns a key code.
///
/// See <Key codes for keyhit()>
///
/// Note:
/// Only Arrows, Esc, Enter and Space are currently working properly.
static int getkey(void) {
#ifndef _WIN32
	int cnt = kbhit(); // for ANSI escapes processing
#endif
	int k = getch();
	switch (k) {
	case 0: {
		int kk;
		switch (kk = getch()) {
		case 71:
			return KEY_NUMPAD7;
		case 72:
			return KEY_NUMPAD8;
		case 73:
			return KEY_NUMPAD9;
		case 75:
			return KEY_NUMPAD4;
		case 77:
			return KEY_NUMPAD6;
		case 79:
			return KEY_NUMPAD1;
		case 80:
			return KEY_NUMPAD2;
		case 81:
			return KEY_NUMPAD3;
		case 82:
			return KEY_NUMPAD0;
		case 83:
			return KEY_NUMDEL;
		default:
			return kk - 59 + KEY_F1; // Function keys
		}
	}
	case 224: {
		int kk;
		switch (kk = getch()) {
		case 71:
			return KEY_HOME;
		case 72:
			return KEY_UP;
		case 73:
			return KEY_PGUP;
		case 75:
			return KEY_LEFT;
		case 77:
			return KEY_RIGHT;
		case 79:
			return KEY_END;
		case 80:
			return KEY_DOWN;
		case 81:
			return KEY_PGDOWN;
		case 82:
			return KEY_INSERT;
		case 83:
			return KEY_DELETE;
		default:
			return kk - 123 + KEY_F1; // Function keys
		}
	}
	case 13:
		return KEY_ENTER;
#ifdef _WIN32
		case 27: return KEY_ESCAPE;
#else // _WIN32
	case 155: // single-character CSI
	case 27: {
		// Process ANSI escape sequences
		if (cnt >= 3 && getch() == '[') {
			switch (k = getch()) {
			case 'A':
				return KEY_UP;
			case 'B':
				return KEY_DOWN;
			case 'C':
				return KEY_RIGHT;
			case 'D':
				return KEY_LEFT;
			}
		} else
			return KEY_ESCAPE;
	}
#endif // _WIN32
	default:
		return k;
	}
}

/// Function: nb_getch
/// Non-blocking getch(). Returns 0 if no key was pressed.
static int nb_getch(void) {
	if (kbhit())
		return getch();
	else
		return 0;
}

/// Function: getANSIColor
/// Return ANSI color escape sequence for specified number 0-15.
///
/// See <Color Codes>
static std::string getANSIColor(const int c) {
	switch (c) {
	case BLACK:
		return ANSI_BLACK;
	case BLUE:
		return ANSI_BLUE; // non-ANSI
	case GREEN:
		return ANSI_GREEN;
	case CYAN:
		return ANSI_CYAN; // non-ANSI
	case RED:
		return ANSI_RED; // non-ANSI
	case MAGENTA:
		return ANSI_MAGENTA;
	case BROWN:
		return ANSI_BROWN;
	case GREY:
		return ANSI_GREY;
	case DARKGREY:
		return ANSI_DARKGREY;
	case LIGHTBLUE:
		return ANSI_LIGHTBLUE; // non-ANSI
	case LIGHTGREEN:
		return ANSI_LIGHTGREEN;
	case LIGHTCYAN:
		return ANSI_LIGHTCYAN; // non-ANSI;
	case LIGHTRED:
		return ANSI_LIGHTRED; // non-ANSI;
	case LIGHTMAGENTA:
		return ANSI_LIGHTMAGENTA;
	case YELLOW:
		return ANSI_YELLOW; // non-ANSI
	case WHITE:
		return ANSI_WHITE;
	default:
		return "";
	}
}

/// Function: getANSIBackgroundColor
/// Return ANSI background color escape sequence for specified number 0-15.
///
/// See <Color Codes>
static std::string getANSIBackgroundColor(const int c) {
	switch (c) {
	case BLACK:
		return ANSI_BACKGROUND_BLACK;
	case BLUE:
		return ANSI_BACKGROUND_BLUE;
	case GREEN:
		return ANSI_BACKGROUND_GREEN;
	case CYAN:
		return ANSI_BACKGROUND_CYAN;
	case RED:
		return ANSI_BACKGROUND_RED;
	case MAGENTA:
		return ANSI_BACKGROUND_MAGENTA;
	case BROWN:
		return ANSI_BACKGROUND_YELLOW;
	case GREY:
		return ANSI_BACKGROUND_WHITE;
	default:
		return "";
	}
}

/// Function: setColor
/// Change color specified by number (Windows / QBasic colors).
/// Don't change the background color
///
/// See <Color Codes>
static void setStreamColor(FILE *stream, const int c) {
#if defined(_WIN32) && !defined(RLUTIL_USE_ANSI)
	HANDLE hConsole = GetStdHandle(
		stream == stderr ? STD_ERROR_HANDLE : STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO csbi;

	GetConsoleScreenBufferInfo(hConsole, &csbi);

	SetConsoleTextAttribute(hConsole, (csbi.wAttributes & 0xFFF0) | (WORD)c); // Foreground colors take up the least significant byte
#else
	RLUTIL_FPRINT(stream, getANSIColor(c));
#endif
}
static void setColor(const int c) {
	setStreamColor(stdout, c);
}

/// Function: setBackgroundColor
/// Change background color specified by number (Windows / QBasic colors).
/// Don't change the foreground color
///
/// See <Color Codes>
static void setBackgroundColor(int c) {
#if defined(_WIN32) && !defined(RLUTIL_USE_ANSI)
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO csbi;

	GetConsoleScreenBufferInfo(hConsole, &csbi);

	SetConsoleTextAttribute(hConsole, (csbi.wAttributes & 0xFF0F) | (((WORD)c) << 4)); // Background colors take up the second-least significant byte
#else
	RLUTIL_PRINT(getANSIBackgroundColor(c));
#endif
}

/// Function: saveDefaultColor
/// Call once to preserve colors for use in resetColor()
/// on Windows without ANSI, no-op otherwise
///
/// See <Color Codes>
/// See <resetColor>
static int saveStreamDefaultColor(FILE *stream) {
#if defined(_WIN32) && !defined(RLUTIL_USE_ANSI)
	static char initialized = 0; // bool
	static WORD attributes;

	if (!initialized) {
		CONSOLE_SCREEN_BUFFER_INFO csbi;
		GetConsoleScreenBufferInfo(
			GetStdHandle(stream == stderr ? STD_ERROR_HANDLE : STD_OUTPUT_HANDLE),
			&csbi);
		attributes = csbi.wAttributes;
		initialized = 1;
	}
	return (int)attributes;
#else
	(void) stream;
	return -1;
#endif
}
static int saveDefaultColor(void) {
	return saveStreamDefaultColor(stdout);
}

/// Function: resetColor
/// Reset color to default
/// Requires a call to saveDefaultColor() to set the defaults
///
/// See <Color Codes>
/// See <setColor>
/// See <saveDefaultColor>
static void resetStreamColor(FILE *stream) {
#if defined(_WIN32) && !defined(RLUTIL_USE_ANSI)
	SetConsoleTextAttribute(
		GetStdHandle(stream == stderr ? STD_ERROR_HANDLE : STD_OUTPUT_HANDLE),
		(WORD)saveDefaultColor());
#else
	RLUTIL_FPRINT(stream, ANSI_ATTRIBUTE_RESET);
#endif
}
static void resetColor(void) {
	resetStreamColor(stdout);
}

/// Function: cls
/// Clears screen, resets all attributes and moves cursor home.
static void cls(void) {
#if defined(_WIN32) && !defined(RLUTIL_USE_ANSI)
	// Based on https://msdn.microsoft.com/en-us/library/windows/desktop/ms682022%28v=vs.85%29.aspx
	const HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	const COORD coordScreen = {0, 0};
	DWORD cCharsWritten;
	CONSOLE_SCREEN_BUFFER_INFO csbi;

	GetConsoleScreenBufferInfo(hConsole, &csbi);
	const DWORD dwConSize = csbi.dwSize.X * csbi.dwSize.Y;
	FillConsoleOutputCharacter(hConsole, (TCHAR)' ', dwConSize, coordScreen, &cCharsWritten);

	GetConsoleScreenBufferInfo(hConsole, &csbi);
	FillConsoleOutputAttribute(hConsole, csbi.wAttributes, dwConSize, coordScreen, &cCharsWritten);

	SetConsoleCursorPosition(hConsole, coordScreen);
#else
	RLUTIL_PRINT(ANSI_CLS);
	RLUTIL_PRINT(ANSI_CURSOR_HOME);
#endif
}

/// Function: locate
/// Sets the cursor position to 1-based x,y.
static void locate(int x, int y) {
#if defined(_WIN32) && !defined(RLUTIL_USE_ANSI)
	COORD coord;
	// TODO: clamping/assert for x/y <= 0?
	coord.X = (SHORT)(x - 1);
	coord.Y = (SHORT)(y - 1); // Windows uses 0-based coordinates
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
#else // _WIN32 || USE_ANSI
#ifdef __cplusplus
	RLUTIL_PRINT("\033[" << y << ";" << x << "H");
#else // __cplusplus
		char buf[32];
		sprintf(buf, "\033[%d;%df", y, x);
		RLUTIL_PRINT(buf);
	#endif // __cplusplus
#endif // _WIN32 || USE_ANSI
}

/// Function: setString
/// Prints the supplied string without advancing the cursor
static void setString(const std::string &str_) {
	const char *const str = str_.data();
	unsigned int len = str_.size();
	RLUTIL_PRINT("\033[" << len << 'D');
}

/// Function: setChar
/// Sets the character at the cursor without advancing the cursor
static void setChar(char ch) {
	const char buf[] = { ch, 0 };
	setString(buf);
}

/// Function: setCursorVisibility
/// Shows/hides the cursor.
static void setCursorVisibility(char visible) {

	RLUTIL_PRINT((visible ? ANSI_CURSOR_SHOW : ANSI_CURSOR_HIDE));
}

/// Function: hidecursor
/// Hides the cursor.
static void hidecursor(void) {
	setCursorVisibility(0);
}

/// Function: showcursor
/// Shows the cursor.
static void showcursor(void) {
	setCursorVisibility(1);
}

/// Function: trows
/// Get the number of rows in the terminal window or -1 on error.
static int trows(void) {
#ifdef _WIN32
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	if (!GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi))
		return -1;
	else
		return csbi.srWindow.Bottom - csbi.srWindow.Top + 1; // Window height
		// return csbi.dwSize.Y; // Buffer height
#else
#ifdef TIOCGSIZE
	struct ttysize ts;
	ioctl(STDIN_FILENO, TIOCGSIZE, &ts);
	return ts.ts_lines;
#elif defined(TIOCGWINSZ)
	struct winsize ts;
	ioctl(STDIN_FILENO, TIOCGWINSZ, &ts);
	return ts.ws_row;
#else // TIOCGSIZE
	return -1;
#endif // TIOCGSIZE
#endif // _WIN32
}

/// Function: tcols
/// Get the number of columns in the terminal window or -1 on error.
static int tcols(void) {
#ifdef _WIN32
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	if (!GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi))
		return -1;
	else
		return csbi.srWindow.Right - csbi.srWindow.Left + 1; // Window width
		// return csbi.dwSize.X; // Buffer width
#else
#ifdef TIOCGSIZE
	struct ttysize ts;
	ioctl(STDIN_FILENO, TIOCGSIZE, &ts);
	return ts.ts_cols;
#elif defined(TIOCGWINSZ)
	struct winsize ts;
	ioctl(STDIN_FILENO, TIOCGWINSZ, &ts);
	return ts.ws_col;
#else // TIOCGSIZE
	return -1;
#endif // TIOCGSIZE
#endif // _WIN32
}

/// Function: anykey
/// Waits until a key is pressed.
/// In C++, it either takes no arguments
/// or a template-type-argument-deduced
/// argument.
/// In C, it takes a const char* representing
/// the message to be displayed, or NULL
/// for no message.
#ifdef __cplusplus
static void anykey() {
	getch();
}

template<class T> void anykey(const T &msg) {
	RLUTIL_PRINT(msg);
#else
static void anykey(RLUTIL_STRING_T msg) {
	if (msg)
		RLUTIL_PRINT(msg);
#endif // __cplusplus
	getch();
}

static void setConsoleTitle(std::string title) {
	const char *true_title = title.c_str();
	RLUTIL_PRINT(ANSI_CONSOLE_TITLE_PRE);
	RLUTIL_PRINT(true_title);
	RLUTIL_PRINT(ANSI_CONSOLE_TITLE_POST);
}

// Classes are here at the end so that documentation is pretty.

struct CursorHider {
	CursorHider() {
		hidecursor();
	}
	~CursorHider() {
		showcursor();
	}
};

