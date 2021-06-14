/*
Imagicle print2fax
Copyright (C) 2021 Lorenzo Monti

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 3
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

//---------------------------------------------------------------------------
#include <vcl.h>
#include <windows.h>
#include <iostream>
#pragma hdrstop

#include "console.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
//---------------------------------------------------------------------------

#define RED L"\x1b[31m"
#define WHITE L"\x1b[37m"
#define YELLOW L"\x1b[33m"
#define BRIGHT L"\x1b[1m"
#define RESET L"\x1b[0m"
//---------------------------------------------------------------------------

void Warning(const UnicodeString &msg)
{
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD dwMode = 0;

	GetConsoleMode(hConsole, &dwMode);
	if (SetConsoleMode(hConsole, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING)) {
		std::wcout << BRIGHT << YELLOW << L"*** WARNING: ";
		std::wcout << WHITE << msg << RESET << std::endl;

		SetConsoleMode(hConsole, dwMode);
	} else {
		std::wcout << L"*** WARNING: ";
		std::wcout << msg << std::endl;
	}
}
//---------------------------------------------------------------------------

void Error(const UnicodeString &msg)
{
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD dwMode = 0;

	GetConsoleMode(hConsole, &dwMode);
	if (SetConsoleMode(hConsole, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING)) {
		std::wcout << BRIGHT << RED << L"*** ERROR: ";
		std::wcout << WHITE << msg << RESET << std::endl;

		SetConsoleMode(hConsole, dwMode);
	} else {
		std::wcout << L"*** ERROR: ";
		std::wcout << msg << std::endl;
	}
}
//---------------------------------------------------------------------------
