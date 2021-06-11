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

#ifndef printerH
#define printerH

#include <vcl.h>
#pragma hdrstop
//---------------------------------------------------------------------------

class WfiPrinter
{
private:
	UnicodeString FEnvironment;
	TResourceStream *FCertBlob;

	bool IsDriverInstalled();
	bool IsMonitorInstalled();
	void InstallCertificateIfMissing();
	void InstallDriverIfMissing(bool quiet);
	void InstallDriver(bool quiet);
	void AddWfiPrinterMonitor();
	void DeleteWfiPrinterMonitor();
	void AddWfiPrinter();
	void DeleteWfiPrinter();
	void RestartSpooler();
	void PurgePrinterJobs();
	void CreateGhostscriptTempFile(const UnicodeString &aFileName, int resId);
	void CleanupGhostscriptTempFile(const UnicodeString &aFileName);

public:
	WfiPrinter();
	virtual ~WfiPrinter();
	void Install(bool quiet);
	void UninstallMonitor();
	void UninstallPrinter();
	void Uninstall();
};
//---------------------------------------------------------------------------

#endif
