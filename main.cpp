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
#pragma hdrstop

#pragma argsused

#include <tchar.h>
#include <stdio.h>

#include "printer.h"
#include "console.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
//---------------------------------------------------------------------------

void usage()
{
	throw Exception(L"usage: printerutil.exe <install [quiet] | uninstallprinter | uninstallmonitor | uninstall>");
}
//---------------------------------------------------------------------------

int wmain(int argc, WCHAR* argv[])
{
	try {
		if (argc < 2)
			usage();

		WfiPrinter printer;

		if (_wcsicmp(argv[1], L"install") == 0) {
			bool quiet = false;

			if (argc < 2 || argc > 3)
				usage();

			if (argc == 3) {
				if (_wcsicmp(argv[2], L"quiet") == 0) {
					quiet = true;
				} else {
					usage();
				}
			}

			printer.Install(quiet);
		} else {
			if (argc > 2)
				usage();

			if (_wcsicmp(argv[1], L"uninstallprinter") == 0)
				printer.UninstallPrinter();
			else if (_wcsicmp(argv[1], L"uninstallmonitor") == 0)
				printer.UninstallMonitor();
			else if (_wcsicmp(argv[1], L"uninstall") == 0)
				printer.Uninstall();
			else {
				usage();
			}
		}
	}
	catch (Exception &e) {
		Error(e.Message);

		return 1;
	}

	return 0;
}
//---------------------------------------------------------------------------

