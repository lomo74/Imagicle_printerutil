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
#include <windows.h>
#include <wincrypt.h>
#pragma hdrstop

#include "printer.h"
#include "console.h"
#include "resourceids.h"

#include <JclSysInfo.hpp>
#include <JclSvcCtrl.hpp>
//---------------------------------------------------------------------------
#pragma package(smart_init)
//---------------------------------------------------------------------------

wchar_t const *pMonitorName = L"Imagicle print2fax port monitor";
wchar_t const *pDllName = L"wfimon.dll";
wchar_t const *pDriverName = L"Ghostscript PDF";
wchar_t const *pPrinterName = L"Imagicle print2fax";
wchar_t const *pSpooler = L"spooler";
//---------------------------------------------------------------------------

WfiPrinter::WfiPrinter()
{
	if (IsWindows64()) {
		FEnvironment = L"Windows x64";
	} else {
		FEnvironment = L"Windows NT x86";
	}

	NativeUInt hModule = reinterpret_cast<NativeUInt>(GetModuleHandle(NULL));

	WCHAR *szRaw = _wcsdup(L"RAW");

	try {
		FCertBlob = new TResourceStream(hModule, ID_CERT, szRaw);
	}
	__finally {
		free(szRaw);
	}
}
//---------------------------------------------------------------------------

WfiPrinter::~WfiPrinter()
{
	delete FCertBlob;
}
//---------------------------------------------------------------------------

bool WfiPrinter::IsDriverInstalled()
{
	UnicodeString err;
	DWORD pcbNeeded, pcReturned;

	if (EnumPrinterDriversW(NULL, FEnvironment.c_str(), 3, NULL, 0, &pcbNeeded, &pcReturned))
		return false;

	if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
		BYTE *buffer = new BYTE[pcbNeeded];

		try {
			if (EnumPrinterDriversW(NULL, FEnvironment.c_str(), 3, buffer, pcbNeeded, &pcbNeeded, &pcReturned)) {
				DRIVER_INFO_3W *pInfo = reinterpret_cast<DRIVER_INFO_3W *>(buffer);

				for (int i = 0; i < pcReturned; i++, pInfo++) {
					if (_wcsicmp(pInfo->pName, pDriverName) == 0)
						return true;
				}

				return false;
			}
		}
		__finally {
			delete[] buffer;
		}
	}

	err.printf(L"call to EnumPrinterDriversW failed. GetLastError=0x%08X", GetLastError());
	throw Exception(err);
}
//---------------------------------------------------------------------------

bool WfiPrinter::IsMonitorInstalled()
{
	UnicodeString err;
	DWORD pcbNeeded, pcReturned;

	if (EnumMonitorsW(NULL, 2, NULL, 0, &pcbNeeded, &pcReturned))
		return false;

	if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
		BYTE *buffer = new BYTE[pcbNeeded];

		try {
			if (EnumMonitorsW(NULL, 2, buffer, pcbNeeded, &pcbNeeded, &pcReturned)) {
				MONITOR_INFO_2W *pInfo = reinterpret_cast<MONITOR_INFO_2W *>(buffer);

				for (int i = 0; i < pcReturned; i++, pInfo++) {
					if (_wcsicmp(pInfo->pName, pMonitorName) == 0)
						return true;
				}

				return false;
			}
		}
		__finally {
			delete[] buffer;
		}
	}

	err.printf(L"call to EnumMonitorsW failed. GetLastError=0x%08X", GetLastError());
	throw Exception(err);
}
//---------------------------------------------------------------------------

void WfiPrinter::InstallCertificateIfMissing()
{
	UnicodeString err;

	PCCERT_CONTEXT hCertContext = NULL;
	HCERTSTORE hStore = NULL;

	try {
		BYTE hash[20];
		DWORD cbHash = sizeof(hash);
		CRYPT_HASH_BLOB hash_blob = { sizeof(hash), hash };

		hCertContext = CertCreateCertificateContext(
			PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
			static_cast<const BYTE *>(FCertBlob->Memory),
			FCertBlob->Size
		);

		if (!hCertContext) {
			err.printf(L"call to CertCreateCertificateContext failed. GetLastError=0x%08X", GetLastError());
			throw Exception(err);
		}

		if (!CertGetCertificateContextProperty(hCertContext, CERT_HASH_PROP_ID, hash, &cbHash)) {
			err.printf(L"call to CertGetCertificateContextProperty failed. GetLastError=0x%08X", GetLastError());
			throw Exception(err);
		}

		hStore = CertOpenStore(
			CERT_STORE_PROV_SYSTEM,
			0,
			NULL,
			CERT_SYSTEM_STORE_LOCAL_MACHINE,
			L"TrustedPublisher"
		);

		if (!hStore) {
			err.printf(L"call to CertOpenStore failed. GetLastError=0x%08X", GetLastError());
			throw Exception(err);
		}

		PCCERT_CONTEXT pExistingCert = CertFindCertificateInStore(
			hStore,
			PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
			0,
			CERT_FIND_HASH,
			&hash_blob,
			NULL
		);

		if (pExistingCert) {
			CertFreeCertificateContext(pExistingCert);
		} else {
			if (!CertAddCertificateContextToStore(hStore, hCertContext, CERT_STORE_ADD_NEWER, NULL)) {
				err.printf(L"call to CertAddCertificateContextToStore failed. GetLastError=0x%08X", GetLastError());
				throw Exception(err);
			}
		}
	}
	__finally {
		if (hStore)
			CertCloseStore(hStore, CERT_CLOSE_STORE_CHECK_FLAG);

		if (hCertContext)
			CertFreeCertificateContext(hCertContext);
	}
}
//---------------------------------------------------------------------------

void WfiPrinter::InstallDriverIfMissing(bool quiet)
{
	if (!IsDriverInstalled()) {
		InstallDriver(quiet);
	}
}
//---------------------------------------------------------------------------

void WfiPrinter::InstallDriver(bool quiet)
{
	UnicodeString err;

	WCHAR *pName = _wcsdup(pDriverName);

	WCHAR pszDestInfPath[512];
	ULONG cchDestInfPath = 512;
	UnicodeString szTemp, szInfFile, szCatFile, szPpdFile;

	try {
		HRESULT res;

		const DWORD ccTemp = MAX_PATH + 1;

		szTemp.SetLength(ccTemp);

		DWORD dwRet = GetTempPathW(ccTemp, szTemp.c_str());

		if (dwRet == 0 || dwRet > ccTemp) {
			err.printf(L"call to GetTempPathW failed. GetLastError=0x%08X", GetLastError());
			throw Exception(err);
		}

		szTemp.SetLength(dwRet);

		TGUID aGuid = TGUIDHelper::NewGuid();

		szTemp = IncludeTrailingPathDelimiter(szTemp) + GUIDToString(aGuid);

		if (!CreateDirectoryW(szTemp.c_str(), NULL)) {
			err.printf(L"call to CreateDirectoryW failed. GetLastError=0x%08X", GetLastError());
			throw Exception(err);
		}

		szInfFile = szTemp + L"\\ghostpdf.inf";
		szCatFile = szTemp + L"\\ghostpdf.cat";
		szPpdFile = szTemp + L"\\ghostpdf.ppd";

		CreateGhostscriptTempFile(szInfFile, ID_GHOSTPDF_INF);
		CreateGhostscriptTempFile(szCatFile, ID_GHOSTPDF_CAT);
		CreateGhostscriptTempFile(szPpdFile, ID_GHOSTPDF_PPD);

		DWORD dwFlags = quiet
			? UPDP_SILENT_UPLOAD | UPDP_UPLOAD_ALWAYS
			: UPDP_UPLOAD_ALWAYS;

		res = UploadPrinterDriverPackageW(
			NULL,
			szInfFile.c_str(),
			FEnvironment.c_str(),
			dwFlags,
			GetDesktopWindow(),
			pszDestInfPath,
			&cchDestInfPath
		);

		if (FAILED(res)) {
			err.printf(L"call to UploadPrinterDriverPackageW failed. HRESULT=0x%08X", res);
			throw Exception(err);
		}

		res = InstallPrinterDriverFromPackageW(NULL, pszDestInfPath, pName, FEnvironment.c_str(), 0);

		if (FAILED(res)) {
			err.printf(L"call to UploadPrinterDriverPackageW failed. HRESULT=0x%08X", res);
			throw Exception(err);
		}
	}
	__finally {
		free(pName);

		CleanupGhostscriptTempFile(szInfFile);
		CleanupGhostscriptTempFile(szCatFile);
		CleanupGhostscriptTempFile(szPpdFile);

		if (szTemp.Length() > 0)
			RemoveDirectoryW(szTemp.c_str());
	}
}
//---------------------------------------------------------------------------

void WfiPrinter::CreateGhostscriptTempFile(const UnicodeString &aFileName, int resId)
{
	NativeUInt hModule = reinterpret_cast<NativeUInt>(GetModuleHandle(NULL));

	WCHAR *szRaw = _wcsdup(L"RAW");

	try {
		TFileStream *pFile = new TFileStream(aFileName, fmCreate);
		TResourceStream *pStream = new TResourceStream(hModule, resId, szRaw);

		try {
			pFile->CopyFrom(pStream);
		}
		__finally {
			delete pFile;
			delete pStream;
		}
	}
	__finally {
		free(szRaw);
	}
}
//---------------------------------------------------------------------------

void WfiPrinter::CleanupGhostscriptTempFile(const UnicodeString &aFileName)
{
	if (aFileName.Length() > 0)
		DeleteFileW(aFileName.c_str());
}
//---------------------------------------------------------------------------

void WfiPrinter::AddWfiPrinterMonitor()
{
	UnicodeString err;

	MONITOR_INFO_2W minfo = { 0 };

	minfo.pName = _wcsdup(pMonitorName);
	minfo.pEnvironment = NULL;
	minfo.pDLLName = _wcsdup(pDllName);

	try {
		if (!AddMonitorW(NULL, 2, reinterpret_cast<LPBYTE>(&minfo))) {
			err.printf(L"call to AddMonitorW failed. GetLastError=0x%08X", GetLastError());
			throw Exception(err);
		}
	}
	__finally {
		free(minfo.pName);
		free(minfo.pDLLName);
	}
}
//---------------------------------------------------------------------------

void WfiPrinter::DeleteWfiPrinterMonitor()
{
	UnicodeString err;
	LPWSTR pName = _wcsdup(pMonitorName);

	try {
		if (!DeleteMonitorW(NULL, FEnvironment.c_str(), pName)) {
			err.printf(L"call to DeleteMonitorW failed. GetLastError=0x%08X", GetLastError());
			throw Exception(err);
		}
	}
	__finally {
		free(pName);
	}
}
//---------------------------------------------------------------------------

void WfiPrinter::AddWfiPrinter()
{
	UnicodeString err;

	PRINTER_INFO_2W pinfo = { 0 };

	pinfo.pPrinterName = _wcsdup(pPrinterName);
	pinfo.pPortName = _wcsdup(L"WFI:");
	pinfo.pDriverName = _wcsdup(pDriverName);
	pinfo.pPrintProcessor = _wcsdup(L"WinPrint");
	pinfo.pDatatype = _wcsdup(L"RAW");
	pinfo.pComment = _wcsdup(L"Imagicle print2fax Printer");
	pinfo.Priority = 1;
	pinfo.DefaultPriority = 1;

	try {
		HANDLE hPrinter = AddPrinterW(NULL, 2, reinterpret_cast<LPBYTE>(&pinfo));

		if (!hPrinter) {
			err.printf(L"call to AddPrinterW failed. GetLastError=0x%08X", GetLastError());
			throw Exception(err);
		}

		if (!ClosePrinter(hPrinter)) {
			err.printf(L"call to ClosePrinter failed. GetLastError=0x%08X", GetLastError());
			throw Exception(err);
		}
	}
	__finally {
		free(pinfo.pPrinterName);
		free(pinfo.pPortName);
		free(pinfo.pDriverName);
		free(pinfo.pPrintProcessor);
		free(pinfo.pDatatype);
		free(pinfo.pComment);
	}
}
//---------------------------------------------------------------------------

void WfiPrinter::DeleteWfiPrinter()
{
	UnicodeString err;
	LPWSTR pName = _wcsdup(pPrinterName);

	HANDLE hPrinter = NULL;

	try {
		PRINTER_DEFAULTS defaults = { NULL, NULL, PRINTER_ALL_ACCESS };

		if (!OpenPrinterW(pName, &hPrinter, &defaults)) {
			err.printf(L"call to OpenPrinterW failed. GetLastError=0x%08X", GetLastError());
			throw Exception(err);
		}

		if (!DeletePrinter(hPrinter)) {
			err.printf(L"call to DeleteMonitorW failed. GetLastError=0x%08X", GetLastError());
			throw Exception(err);
		}
	}
	__finally {
		free(pName);

		if (hPrinter)
			ClosePrinter(hPrinter);
	}
}
//---------------------------------------------------------------------------

void WfiPrinter::RestartSpooler()
{
	StopServiceByName(NULL, pSpooler);
	StartServiceByName(NULL, pSpooler);
}
//---------------------------------------------------------------------------

void WfiPrinter::PurgePrinterJobs()
{
	UnicodeString err;
	LPWSTR pName = _wcsdup(pPrinterName);

	HANDLE hPrinter = NULL;

	try {
		PRINTER_DEFAULTS defaults = { NULL, NULL, PRINTER_ALL_ACCESS };

		if (!OpenPrinterW(pName, &hPrinter, &defaults)) {
			err.printf(L"call to OpenPrinterW failed. GetLastError=0x%08X", GetLastError());
			throw Exception(err);
		}

		if (!SetPrinter(hPrinter, 0, NULL, PRINTER_CONTROL_PURGE)) {
			err.printf(L"call to OpenPrinterW failed. GetLastError=0x%08X", GetLastError());
			throw Exception(err);
		}
	}
	__finally {
		free(pName);

		if (hPrinter)
			ClosePrinter(hPrinter);
	}
}
//---------------------------------------------------------------------------

void WfiPrinter::Install(bool quiet)
{
	RestartSpooler();

	if (quiet) {
		InstallCertificateIfMissing();
	}

	InstallDriverIfMissing(quiet);

	if (!IsMonitorInstalled()) {
		AddWfiPrinterMonitor();
		RestartSpooler();
	}

	AddWfiPrinter();
}
//---------------------------------------------------------------------------

void WfiPrinter::UninstallMonitor()
{
	DeleteWfiPrinterMonitor();
}
//---------------------------------------------------------------------------

void WfiPrinter::UninstallPrinter()
{
	PurgePrinterJobs();
	DeleteWfiPrinter();
}
//---------------------------------------------------------------------------

void WfiPrinter::Uninstall()
{
	RestartSpooler();

	try {
		UninstallPrinter();
	}
	catch (Exception &e) {
		Warning(e.Message);
	}

	RestartSpooler();

	try {
		UninstallMonitor();
	}
	catch (Exception &e) {
		Warning(e.Message);
	}
}
//---------------------------------------------------------------------------

