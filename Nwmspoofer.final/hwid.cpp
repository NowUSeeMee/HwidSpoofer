#pragma warning(disable:4996)

#include <iostream>
#include <sstream>
#include <Windows.h>

#include <string.h>
#include <string>
#include <array>

#include <Iphlpapi.h>
#include <Assert.h>
#include <iomanip>
#include <conio.h>
#include <intrin.h>
#include <comdef.h>
#include <Wbemidl.h>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "wbemuuid.lib")

#include "encrypt/md5.h"
#include "encrypt/xor.h"
#include "hwid.h"

std::string HWID = _xor_("a");

using namespace std;

char* getVolumeId() {
	DWORD VolumeSerialNumber = 0;
	// Windows 11 compatible: use wide-char version explicitly
	GetVolumeInformationA("C:\\", NULL, NULL, &VolumeSerialNumber, NULL, NULL, NULL, NULL);
	char* str = new char[16];
	sprintf(str, "%d", VolumeSerialNumber);
	return str;
}

char* getMAC() {
	PIP_ADAPTER_INFO AdapterInfo;
	DWORD dwBufLen = sizeof(IP_ADAPTER_INFO);
	char* mac_addr = (char*)malloc(18);

	AdapterInfo = (IP_ADAPTER_INFO*)malloc(sizeof(IP_ADAPTER_INFO));
	if (AdapterInfo == NULL) {
		free(mac_addr);
		return NULL;
	}
	if (GetAdaptersInfo(AdapterInfo, &dwBufLen) == ERROR_BUFFER_OVERFLOW) {
		free(AdapterInfo);
		AdapterInfo = (IP_ADAPTER_INFO*)malloc(dwBufLen);
		if (AdapterInfo == NULL) {
			free(mac_addr);
			return NULL;
		}
	}
	if (GetAdaptersInfo(AdapterInfo, &dwBufLen) == NO_ERROR) {
		PIP_ADAPTER_INFO pAdapterInfo = AdapterInfo;
		do {
			sprintf(mac_addr, "%02X:%02X:%02X:%02X:%02X:%02X",
				pAdapterInfo->Address[0], pAdapterInfo->Address[1],
				pAdapterInfo->Address[2], pAdapterInfo->Address[3],
				pAdapterInfo->Address[4], pAdapterInfo->Address[5]);
			pAdapterInfo = pAdapterInfo->Next;
		} while (pAdapterInfo);
	}
	free(AdapterInfo);
	return mac_addr;
}

string getProductId() {
	HKEY keyHandle;
	WCHAR rgValue[1024];
	DWORD size1;
	DWORD Type;

	// Windows 11 changed the registry value name casing
	// Try "ProductId" first (Win10), then "ProductID" (Win11)
	if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0, KEY_QUERY_VALUE | KEY_WOW64_64KEY, &keyHandle) == ERROR_SUCCESS)
	{
		size1 = sizeof(rgValue);
		LONG result = RegQueryValueExW(keyHandle, L"ProductId", NULL, &Type, (LPBYTE)rgValue, &size1);
		if (result != ERROR_SUCCESS) {
			// Fallback for Windows 11 which may use different casing
			size1 = sizeof(rgValue);
			result = RegQueryValueExW(keyHandle, L"ProductID", NULL, &Type, (LPBYTE)rgValue, &size1);
		}
		RegCloseKey(keyHandle);
		if (result == ERROR_SUCCESS) {
			wstring rgValueCh = rgValue;
			string convertRgVal(rgValueCh.begin(), rgValueCh.end());
			return convertRgVal;
		}
	}
	return "UNKNOWN-PRODUCT-ID";
}

string getCurrentBuild() {
	HKEY keyHandle;
	WCHAR rgValue[1024];
	DWORD size1;
	DWORD Type;

	if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0, KEY_QUERY_VALUE | KEY_WOW64_64KEY, &keyHandle) == ERROR_SUCCESS)
	{
		// Try CurrentBuild first
		size1 = sizeof(rgValue);
		LONG result = RegQueryValueExW(keyHandle, L"CurrentBuild", NULL, &Type, (LPBYTE)rgValue, &size1);
		if (result != ERROR_SUCCESS) {
			// Fallback to CurrentBuildNumber (used by some Win11 editions)
			size1 = sizeof(rgValue);
			result = RegQueryValueExW(keyHandle, L"CurrentBuildNumber", NULL, &Type, (LPBYTE)rgValue, &size1);
		}
		RegCloseKey(keyHandle);
		if (result == ERROR_SUCCESS) {
			wstring rgValueCh = rgValue;
			string convertRgVal(rgValueCh.begin(), rgValueCh.end());
			return convertRgVal;
		}
	}
	return "00000";
}

string getMachineId()
{
	string ss;
	ss = "Err_StringIsNull";
	DWORD dwSerial = 0;
	DWORD dwMFL = 0;
	DWORD dwSysFlags = 0;

	// Use the ANSI version explicitly for compatibility
	bool success = GetVolumeInformationA("C:\\", NULL, 0, &dwSerial, &dwMFL, &dwSysFlags, NULL, 0);
	if (!success) {
		ss = "Err_Not_Elevated";
		return ss;
	}
	std::stringstream errorStream;
	errorStream << dwSerial;
	return string(errorStream.str().c_str());
}

string getHWinfo64() {
	HW_PROFILE_INFOA hwProfileInfo;
	if (GetCurrentHwProfileA(&hwProfileInfo)) {

		string a, b, c, d, e, f, g, h;
		string hwid = hwProfileInfo.szHwProfileGuid;

		// Safety check for profile GUID length
		if (hwid.length() < 16) {
			// Fallback: use WMI to get a unique system identifier
			return md5(hwid + getMachineId());
		}

		a = hwid.substr(0, 3); b = hwid.substr(2, 3);
		c = hwid.substr(4, 3); d = hwid.substr(6, 3);
		e = hwid.substr(8, 3); f = hwid.substr(10, 3);
		g = hwid.substr(12, 3); h = hwid.substr(14, 3);

		string mixedHwid = a + c + b + a + e + f + h + g + b + e + c + a + d + f + d;

		mixedHwid.erase(std::remove(mixedHwid.begin(), mixedHwid.end(), '-'), mixedHwid.end());
		mixedHwid.erase(std::remove(mixedHwid.begin(), mixedHwid.end(), '{'), mixedHwid.end());
		mixedHwid.erase(std::remove(mixedHwid.begin(), mixedHwid.end(), '}'), mixedHwid.end());

		string hashedHwid = md5(mixedHwid);

		string prod = getProductId();
		if (prod.length() < 6) {
			prod = "000000000000";
		}
		string j, k, l, m;

		j = prod.substr(0, 3); k = prod.substr(1, 3);
		l = prod.substr(2, 3); m = prod.substr(3, 3);

		string mixedProduct = j + k + m + j + k + l + l + m + k + j;

		mixedProduct.erase(std::remove(mixedProduct.begin(), mixedProduct.end(), '-'), mixedProduct.end());
		mixedProduct.erase(std::remove(mixedProduct.begin(), mixedProduct.end(), '{'), mixedProduct.end());
		mixedProduct.erase(std::remove(mixedProduct.begin(), mixedProduct.end(), '}'), mixedProduct.end());

		string hashedProduct = md5(mixedProduct);

		string cubu = getCurrentBuild();
		string hashedCubu = md5(cubu);

		string mcid = getMachineId();
		if (mcid.length() < 7) {
			mcid = "0000000";
		}
		string x, y, z;

		x = mcid.substr(0, 3); y = mcid.substr(2, 3);
		z = mcid.substr(4, 3);

		string machineMixer = x + y + y + x + z + y + x;

		machineMixer.erase(std::remove(machineMixer.begin(), machineMixer.end(), '-'), machineMixer.end());
		machineMixer.erase(std::remove(machineMixer.begin(), machineMixer.end(), '{'), machineMixer.end());
		machineMixer.erase(std::remove(machineMixer.begin(), machineMixer.end(), '}'), machineMixer.end());

		string hashedMachineId = md5(machineMixer);

		HWID = md5(md5(hashedCubu + hashedMachineId + hashedHwid + hashedMachineId + hashedProduct + hashedCubu));

		return HWID;
	}

	// Fallback: if GetCurrentHwProfile fails on Windows 11
	// (can happen with certain security policies), build HWID from other sources
	string fallbackHwid = md5(getProductId() + getCurrentBuild() + getMachineId());
	HWID = md5(fallbackHwid);
	return HWID;
}

string GetProcessorId() {
	std::array<int, 4> cpuInfo;
	__cpuid(cpuInfo.data(), 1);
	std::ostringstream buffer;
	buffer
		<< std::uppercase << std::hex << std::setfill('0')
		<< std::setw(8) << cpuInfo.at(3)
		<< std::setw(8) << cpuInfo.at(0);
	return buffer.str();
}

string GetBaseboard() {
	HRESULT hres;
	hres = CoInitializeEx(0, COINIT_MULTITHREADED);
	if (FAILED(hres))
	{
		// COM may already be initialized in a different mode on Win11
		if (hres != RPC_E_CHANGED_MODE) {
			return "NULL";
		}
	}

	// Skip CoInitializeSecurity if COM is already initialized
	// Windows 11 may have stricter security requirements
	hres = CoInitializeSecurity(
		NULL,
		-1,
		NULL,
		NULL,
		RPC_C_AUTHN_LEVEL_DEFAULT,
		RPC_C_IMP_LEVEL_IMPERSONATE,
		NULL,
		EOAC_NONE,
		NULL
	);
	if (FAILED(hres))
	{
		// On Windows 11, CoInitializeSecurity may fail with RPC_E_TOO_LATE
		// if security was already initialized. This is safe to ignore.
		if (hres != RPC_E_TOO_LATE) {
			CoUninitialize();
			return "NULL";
		}
	}

	IWbemLocator* pLoc = NULL;

	hres = CoCreateInstance(
		CLSID_WbemLocator,
		0,
		CLSCTX_INPROC_SERVER,
		IID_IWbemLocator, (LPVOID*)&pLoc);

	if (FAILED(hres))
	{
		CoUninitialize();
		return "NULL";
	}
	IWbemServices* pSvc = NULL;
	hres = pLoc->ConnectServer(
		_bstr_t(L"ROOT\\CIMV2"),
		NULL,
		NULL,
		0,
		NULL,
		0,
		0,
		&pSvc
	);

	if (FAILED(hres))
	{
		pLoc->Release();
		CoUninitialize();
		return "NULL";
	}

	hres = CoSetProxyBlanket(
		pSvc,
		RPC_C_AUTHN_WINNT,
		RPC_C_AUTHZ_NONE,
		NULL,
		RPC_C_AUTHN_LEVEL_CALL,
		RPC_C_IMP_LEVEL_IMPERSONATE,
		NULL,
		EOAC_NONE
	);

	if (FAILED(hres))
	{
		pSvc->Release();
		pLoc->Release();
		CoUninitialize();
		return "NULL";
	}
	IEnumWbemClassObject* pEnumerator = NULL;
	hres = pSvc->ExecQuery(
		bstr_t("WQL"),
		bstr_t("SELECT * FROM Win32_BaseBoard"),
		WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
		NULL,
		&pEnumerator);

	if (FAILED(hres))
	{
		pSvc->Release();
		pLoc->Release();
		CoUninitialize();
		return "NULL";
	}

	IWbemClassObject* pclsObj = NULL;
	ULONG uReturn = 0;

	HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1,
		&pclsObj, &uReturn);

	VARIANT vtProp;

	if (pclsObj != nullptr) {
		hr = pclsObj->Get(L"SerialNumber", 0, &vtProp, 0, 0);
		if (SUCCEEDED(hr) && vtProp.bstrVal != nullptr) {
			wstring test = vtProp.bstrVal;
			string asd(test.begin(), test.end());
			VariantClear(&vtProp);
			pclsObj->Release();

			pSvc->Release();
			pLoc->Release();
			pEnumerator->Release();
			CoUninitialize();
			return asd;
		}
		VariantClear(&vtProp);
		pclsObj->Release();
	}

	pSvc->Release();
	pLoc->Release();
	pEnumerator->Release();
	CoUninitialize();
	return "NULL";
}

// Windows 11 specific: Get TPM version info
string GetTPMVersion() {
	HRESULT hres;
	hres = CoInitializeEx(0, COINIT_MULTITHREADED);
	if (FAILED(hres) && hres != RPC_E_CHANGED_MODE) {
		return "NULL";
	}

	IWbemLocator* pLoc = NULL;
	hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER,
		IID_IWbemLocator, (LPVOID*)&pLoc);
	if (FAILED(hres)) { return "NULL"; }

	IWbemServices* pSvc = NULL;
	hres = pLoc->ConnectServer(_bstr_t(L"ROOT\\CIMV2\\Security\\MicrosoftTpm"),
		NULL, NULL, 0, NULL, 0, 0, &pSvc);
	if (FAILED(hres)) {
		pLoc->Release();
		return "NULL";
	}

	CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
		RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);

	IEnumWbemClassObject* pEnumerator = NULL;
	hres = pSvc->ExecQuery(bstr_t("WQL"), bstr_t("SELECT * FROM Win32_Tpm"),
		WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pEnumerator);
	if (FAILED(hres)) {
		pSvc->Release();
		pLoc->Release();
		return "NULL";
	}

	IWbemClassObject* pclsObj = NULL;
	ULONG uReturn = 0;
	hres = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
	if (pclsObj != nullptr) {
		VARIANT vtProp;
		hres = pclsObj->Get(L"SpecVersion", 0, &vtProp, 0, 0);
		if (SUCCEEDED(hres) && vtProp.bstrVal != nullptr) {
			wstring ws = vtProp.bstrVal;
			string result(ws.begin(), ws.end());
			VariantClear(&vtProp);
			pclsObj->Release();
			pEnumerator->Release();
			pSvc->Release();
			pLoc->Release();
			return result;
		}
		VariantClear(&vtProp);
		pclsObj->Release();
	}

	pEnumerator->Release();
	pSvc->Release();
	pLoc->Release();
	return "NULL";
}