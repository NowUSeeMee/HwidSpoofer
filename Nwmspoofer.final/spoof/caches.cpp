#pragma warning(disable:4996)
#include "caches.h"
#include "..\encrypt\xor.h"
#include "..\hwid.h"
#include <cstdio>
#include <random>

template< typename ... Args >
std::string stringer(Args const& ... args)
{
	std::ostringstream stream;
	using List = int[];
	(void)List {
		0, ((void)(stream << args), 0) ...
	};
	return stream.str();
}

// Generate a proper random hex string of given length
static std::string _randHex(int len) {
	static const char hex[] = "0123456789abcdef";
	std::string result;
	std::mt19937 rng(std::random_device{}());
	std::uniform_int_distribution<int> dist(0, 15);
	for (int i = 0; i < len; ++i) result += hex[dist(rng)];
	return result;
}

// Generate a proper random GUID like {xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}
static std::string _randGUID() {
	return "{" + _randHex(8) + "-" + _randHex(4) + "-" + _randHex(4) + "-" + _randHex(4) + "-" + _randHex(12) + "}";
}

// Generate a random MAC address (valid unicast: first byte even)
static std::string _randMAC() {
	std::mt19937 rng(std::random_device{}());
	std::uniform_int_distribution<int> dist(0, 255);
	char mac[13];
	// Ensure first byte is even (unicast) and not 00
	int b0 = (dist(rng) | 0x02) & 0xFE; // locally administered, unicast
	sprintf(mac, "%02X%02X%02X%02X%02X%02X", b0, dist(rng), dist(rng), dist(rng), dist(rng), dist(rng));
	return std::string(mac);
}

// Generate a random computer name like DESKTOP-XXXXXXX
static std::string _randDesktop() {
	static const char chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
	std::mt19937 rng(std::random_device{}());
	std::uniform_int_distribution<int> dist(0, sizeof(chars) - 2);
	std::string name = "DESKTOP-";
	for (int i = 0; i < 7; ++i) name += chars[dist(rng)];
	return name;
}

static std::string _randomBatName() {
	static const char hex[] = "0123456789abcdef";
	std::string name;
	std::mt19937 rng(std::random_device{}());
	std::uniform_int_distribution<int> dist(0, 15);
	for (int i = 0; i < 16; ++i) name += hex[dist(rng)];
	char* tempDir = nullptr;
	size_t len = 0;
	if (_dupenv_s(&tempDir, &len, "TEMP") == 0 && tempDir) {
		std::string result = std::string(tempDir) + "\\" + name + ".bat";
		free(tempDir);
		return result;
	}
	return "C:\\Windows\\Temp\\" + name + ".bat";
}

void CleanCaches()
{
	// =====================================================================
	// ENHANCED HWID SPOOFING (BATCHED for speed)
	// All REG ADD commands run in a single cmd.exe process.
	// Uses proper GUID/hex formatting instead of %random% (which only
	// generates 16-bit decimal numbers that look obviously fake).
	// =====================================================================

	std::string batPath = _randomBatName();
	FILE* bat = fopen(batPath.c_str(), "w");
	if (!bat) return;

	fprintf(bat, "@echo off\n");

	// Pre-generate all random values so each run is unique
	std::string guid1 = _randGUID(), guid2 = _randGUID(), guid3 = _randGUID();
	std::string guid4 = _randGUID(), guid5 = _randGUID(), guid6 = _randGUID();
	std::string guid7 = _randGUID(), guid8 = _randGUID(), guid9 = _randGUID();
	std::string guid10 = _randGUID(), guid11 = _randGUID(), guid12 = _randGUID();
	std::string hex16a = _randHex(16), hex16b = _randHex(16), hex16c = _randHex(16);
	std::string hex32a = _randHex(32), hex32b = _randHex(32), hex32c = _randHex(32);
	std::string hex24a = _randHex(24), hex24b = _randHex(24);
	std::string hex48a = _randHex(48), hex48b = _randHex(48);
	std::string mac1 = _randMAC(), mac2 = _randMAC(), mac3 = _randMAC();
	std::string desktop = _randDesktop();
	std::string prodId = _randHex(5) + "-" + _randHex(5) + "-" + _randHex(5) + "-" + _randHex(5);

	// Anti-cheat registry cleanup
	fprintf(bat, "reg delete HKLM\\SOFTWARE\\WOW6432Node\\EasyAntiCheat /f 2>nul\n");

	// Spoof Windows Update tracking IDs
	fprintf(bat, "REG ADD HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\WindowsUpdate /v SusClientId /t REG_SZ /d %s /f 2>nul\n", guid1.c_str());
	fprintf(bat, "REG ADD HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\WindowsUpdate /v AccountDomainSid /t REG_SZ /d %s /f 2>nul\n", guid2.c_str());
	fprintf(bat, "REG ADD HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\WindowsUpdate /v PingID /t REG_SZ /d %s /f 2>nul\n", guid3.c_str());
	fprintf(bat, "REG ADD HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\WindowsUpdate /v SusClientIdValidation /t REG_BINARY /d %s /f 2>nul\n", hex48a.c_str());

	// Spoof Machine GUID (CRITICAL - FiveM reads this)
	fprintf(bat, "REG ADD HKLM\\SOFTWARE\\Microsoft\\Cryptography /v MachineGuid /t REG_SZ /d %s /f 2>nul\n", guid4.c_str());
	fprintf(bat, "REG ADD HKLM\\SOFTWARE\\Microsoft\\Cryptography /v GUID /t REG_SZ /d %s /f 2>nul\n", guid5.c_str());

	// Spoof Hardware Profile GUID
	fprintf(bat, "REG ADD \"HKLM\\SYSTEM\\CurrentControlSet\\Control\\IDConfigDB\\Hardware Profiles\\0001\" /v HwProfileGuid /t REG_SZ /d %s /f 2>nul\n", guid6.c_str());
	fprintf(bat, "REG ADD \"HKLM\\SYSTEM\\CurrentControlSet\\Control\\IDConfigDB\\Hardware Profiles\\0001\" /v GUID /t REG_SZ /d %s /f 2>nul\n", guid7.c_str());

	// Spoof Hardware Config
	fprintf(bat, "REG ADD HKLM\\SYSTEM\\HardwareConfig /v LastConfig /t REG_SZ /d %s /f 2>nul\n", guid8.c_str());
	fprintf(bat, "REG ADD HKLM\\SYSTEM\\HardwareConfig\\Current /v BaseBoardProduct /t REG_SZ /d BASE-%s /f 2>nul\n", hex16a.c_str());

	// Spoof BIOS BaseBoardProduct
	fprintf(bat, "REG ADD HKLM\\HARDWARE\\DESCRIPTION\\System\\BIOS /v BaseBoardProduct /t REG_SZ /d BASE-%s /f 2>nul\n", hex16a.c_str());

	// Spoof Computer Hardware ID
	fprintf(bat, "REG ADD HKLM\\SYSTEM\\CurrentControlSet\\Control\\SystemInformation /v ComputerHardwareId /t REG_SZ /d %s /f 2>nul\n", guid9.c_str());
	fprintf(bat, "REG ADD HKLM\\SYSTEM\\CurrentControlSet\\Control\\SystemInformation /v ComputerHardwareIds /t REG_SZ /d %s /f 2>nul\n", guid10.c_str());

	// Spoof SQMClient Machine ID
	fprintf(bat, "REG ADD HKLM\\SOFTWARE\\Microsoft\\SQMClient /v MachineId /t REG_SZ /d %s /f 2>nul\n", guid11.c_str());

	// Spoof Product ID
	fprintf(bat, "REG ADD \"HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\" /v ProductId /t REG_SZ /d %s /f 2>nul\n", prodId.c_str());
	fprintf(bat, "REG ADD \"HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\" /v ProductID /t REG_SZ /d %s /f 2>nul\n", prodId.c_str());

	// Spoof Build info
	fprintf(bat, "REG ADD \"HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\" /v BuildGUID /t REG_SZ /d %s /f 2>nul\n", guid12.c_str());

	// NOTE: DigitalProductId spoofing in batch is REMOVED.
	// WMI crashes if DigitalProductId lengths are truncated.
	// They are already safely spoofed natively in CleanProcess() by replacing bytes while retaining original length.
	// Spoof NVIDIA identifiers
	fprintf(bat, "REG ADD \"HKLM\\SOFTWARE\\NVIDIA Corporation\\Global\" /v ClientUUID /t REG_SZ /d %s /f 2>nul\n", _randGUID().c_str());
	fprintf(bat, "REG ADD \"HKLM\\SOFTWARE\\NVIDIA Corporation\\Global\" /v PersistenceIdentifier /t REG_SZ /d %s /f 2>nul\n", _randGUID().c_str());
	fprintf(bat, "REG ADD \"HKLM\\SOFTWARE\\NVIDIA Corporation\\Global\\CoProcManager\" /v ChipsetMatchID /t REG_SZ /d %s /f 2>nul\n", _randGUID().c_str());

	// Spoof GPU driver info
	fprintf(bat, "REG ADD HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0000 /v _DriverProviderInfo /t REG_SZ /d %s /f 2>nul\n", _randGUID().c_str());
	fprintf(bat, "REG ADD HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0000 /v UserModeDriverGUID /t REG_SZ /d %s /f 2>nul\n", _randGUID().c_str());

	// Spoof SCSI/Disk identifiers
	fprintf(bat, "REG ADD \"HKLM\\HARDWARE\\DEVICEMAP\\Scsi\\Scsi Port 0\\Scsi Bus 0\\Target Id 0\\Logical Unit Id 0\" /v Identifier /t REG_SZ /d VBOX_HARDDISK_%s /f 2>nul\n", hex16b.c_str());
	fprintf(bat, "REG ADD \"HKLM\\HARDWARE\\DEVICEMAP\\Scsi\\Scsi Port 1\\Scsi Bus 0\\Target Id 0\\Logical Unit Id 0\" /v Identifier /t REG_SZ /d VBOX_HARDDISK_%s /f 2>nul\n", hex16c.c_str());

	// Spoof Disk Peripheral identifiers
	fprintf(bat, "REG ADD HKLM\\HARDWARE\\DESCRIPTION\\System\\MultifunctionAdapter\\0\\DiskController\\0\\DiskPeripheral\\0 /v Identifier /t REG_SZ /d %s /f 2>nul\n", hex16a.c_str());
	fprintf(bat, "REG ADD HKLM\\HARDWARE\\DESCRIPTION\\System\\MultifunctionAdapter\\0\\DiskController\\0\\DiskPeripheral\\1 /v Identifier /t REG_SZ /d %s /f 2>nul\n", hex16b.c_str());

	// Spoof Video ID
	fprintf(bat, "REG ADD HKLM\\SYSTEM\\ControlSet001\\Services\\BasicDisplay\\Video /v VideoID /t REG_SZ /d %s /f 2>nul\n", _randGUID().c_str());

	// NOTE: TPM data and Direct3D WHQLClass batch spoofing REMOVED.
	// CleanProcess() natively spoofs them correctly without truncating sizes.
	// Spoof Tracing GUID
	fprintf(bat, "REG ADD \"HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Tracing\\Microsoft\\Profile\\Profile\" /v Guid /t REG_SZ /d %s /f 2>nul\n", _randGUID().c_str());

	// Spoof Windows Defender Reporting MachineId
	fprintf(bat, "REG ADD \"HKLM\\SOFTWARE\\Microsoft\\Windows Defender\\Reporting\" /v MachineId /t REG_SZ /d %s /f 2>nul\n", _randGUID().c_str());

	// Spoof WINEVT channel owner
	fprintf(bat, "REG ADD HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\WINEVT\\Channels\\Microsoft-Windows-Kernel-EventTracing/Admin /v OwningPublisher /t REG_SZ /d %s /f 2>nul\n", _randGUID().c_str());

	// =====================================================================
	// FiveM-SPECIFIC REGISTRY SPOOFING
	// =====================================================================

	// Spoof IDE/SCSI disk friendly names
	fprintf(bat, "REG ADD HKLM\\SYSTEM\\CurrentControlSet\\Enum\\IDE /v FriendlyName /t REG_SZ /d DISK_%s /f 2>nul\n", hex16a.c_str());
	fprintf(bat, "REG ADD HKLM\\SYSTEM\\CurrentControlSet\\Enum\\SCSI /v FriendlyName /t REG_SZ /d DISK_%s /f 2>nul\n", hex16b.c_str());

	// Spoof Disk\\Enum entries
	fprintf(bat, "REG ADD HKLM\\SYSTEM\\CurrentControlSet\\Services\\Disk\\Enum /v 0 /t REG_SZ /d IDE\\DISK_%s /f 2>nul\n", hex32a.c_str());
	fprintf(bat, "REG ADD HKLM\\SYSTEM\\CurrentControlSet\\Services\\Disk\\Enum /v 1 /t REG_SZ /d IDE\\DISK_%s /f 2>nul\n", hex32b.c_str());

	// Spoof MAC addresses on network adapters (proper hex MAC format)
	fprintf(bat, "REG ADD \"HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e972-e325-11ce-bfc1-08002be10318}\\0000\" /v NetworkAddress /t REG_SZ /d %s /f 2>nul\n", mac1.c_str());
	fprintf(bat, "REG ADD \"HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e972-e325-11ce-bfc1-08002be10318}\\0001\" /v NetworkAddress /t REG_SZ /d %s /f 2>nul\n", mac2.c_str());
	fprintf(bat, "REG ADD \"HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e972-e325-11ce-bfc1-08002be10318}\\0002\" /v NetworkAddress /t REG_SZ /d %s /f 2>nul\n", mac3.c_str());

	// NOTE: MountedDevices spoofing REMOVED from batch script.
	// It's handled natively in CleanProcess() via SpoofBinary() which writes
	// proper random binary bytes. The batch REG ADD writes hex strings which
	// is the wrong format and conflicts with the native spoof.

	// Spoof BIOS information (FiveM reads these via WMI -> registry)
	fprintf(bat, "REG ADD HKLM\\HARDWARE\\DESCRIPTION\\System\\BIOS /v SystemManufacturer /t REG_SZ /d Manufacturer-%s /f 2>nul\n", _randHex(8).c_str());
	fprintf(bat, "REG ADD HKLM\\HARDWARE\\DESCRIPTION\\System\\BIOS /v SystemProductName /t REG_SZ /d Product-%s /f 2>nul\n", _randHex(8).c_str());
	fprintf(bat, "REG ADD HKLM\\HARDWARE\\DESCRIPTION\\System\\BIOS /v BIOSVendor /t REG_SZ /d Vendor-%s /f 2>nul\n", _randHex(8).c_str());
	fprintf(bat, "REG ADD HKLM\\HARDWARE\\DESCRIPTION\\System\\BIOS /v BIOSVersion /t REG_SZ /d %s /f 2>nul\n", _randHex(8).c_str());
	fprintf(bat, "REG ADD HKLM\\HARDWARE\\DESCRIPTION\\System\\BIOS /v BaseBoardManufacturer /t REG_SZ /d BoardMfg-%s /f 2>nul\n", _randHex(8).c_str());
	fprintf(bat, "REG ADD HKLM\\HARDWARE\\DESCRIPTION\\System\\BIOS /v BaseBoardVersion /t REG_SZ /d %s /f 2>nul\n", _randHex(8).c_str());
	fprintf(bat, "REG ADD HKLM\\HARDWARE\\DESCRIPTION\\System\\BIOS /v SystemFamily /t REG_SZ /d Family-%s /f 2>nul\n", _randHex(8).c_str());
	fprintf(bat, "REG ADD HKLM\\HARDWARE\\DESCRIPTION\\System\\BIOS /v SystemVersion /t REG_SZ /d %s /f 2>nul\n", _randHex(8).c_str());
	fprintf(bat, "REG ADD HKLM\\HARDWARE\\DESCRIPTION\\System\\BIOS /v BIOSReleaseDate /t REG_SZ /d 01/01/2024 /f 2>nul\n");

	// Spoof Computer Name (FiveM reads hostname)
	fprintf(bat, "REG ADD HKLM\\SYSTEM\\CurrentControlSet\\Control\\ComputerName\\ActiveComputerName /v ComputerName /t REG_SZ /d %s /f 2>nul\n", desktop.c_str());
	fprintf(bat, "REG ADD HKLM\\SYSTEM\\CurrentControlSet\\Control\\ComputerName\\ComputerName /v ComputerName /t REG_SZ /d %s /f 2>nul\n", desktop.c_str());

	// Spoof TCP/IP hostname (must match ComputerName)
	fprintf(bat, "REG ADD HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters /v Hostname /t REG_SZ /d %s /f 2>nul\n", desktop.c_str());
	fprintf(bat, "REG ADD HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters /v \"NV Hostname\" /t REG_SZ /d %s /f 2>nul\n", desktop.c_str());

	// NOTE: Volume label change REMOVED. Changing the label does NOT change
	// the volume serial number. It only leaves cosmetic evidence the spoofer ran.
	// Volume serial is spoofed via MountedDevices in CleanProcess().

	// Spoof keyboard/mouse trace GUIDs
	fprintf(bat, "REG ADD HKLM\\SYSTEM\\ControlSet001\\Services\\kbdclass\\Parameters /v WppRecorder_TraceGuid /t REG_SZ /d %s /f 2>nul\n", _randGUID().c_str());
	fprintf(bat, "REG ADD HKLM\\SYSTEM\\ControlSet001\\Services\\mouhid\\Parameters /v WppRecorder_TraceGuid /t REG_SZ /d %s /f 2>nul\n", _randGUID().c_str());

	// Clean NVIDIA telemetry cache (leaks GPU identity)
	fprintf(bat, "rmdir /s /q \"%%programdata%%\\NVIDIA Corporation\\NV_Cache\" 2>nul\n");
	fprintf(bat, "rmdir /s /q \"%%localappdata%%\\NVIDIA\\GLCache\" 2>nul\n");
	fprintf(bat, "rmdir /s /q \"%%localappdata%%\\NVIDIA\\DXCache\" 2>nul\n");
	fprintf(bat, "rmdir /s /q \"%%appdata%%\\NVIDIA\" 2>nul\n");

	// Flush DNS
	fprintf(bat, "ipconfig /flushdns >nul 2>nul\n");

	// Clear ARP cache (can leak MAC)
	fprintf(bat, "arp -d * 2>nul\n");

	// Self-delete
	fprintf(bat, "del /f /q \"%%~f0\" 2>nul\n");

	fclose(bat);

	// Run entire batch in one hidden cmd.exe process
	std::string cmd = std::string("cmd.exe /c \"") + batPath + "\"";
	system(cmd.c_str());
}
