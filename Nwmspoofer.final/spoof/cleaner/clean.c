#include "clean.h"
#include <intrin.h>

int CleanProcess() {
    // Use RDTSC for high-entropy seed instead of GetTickCount (only ~15ms resolution)
    srand((unsigned int)__rdtsc());
    LoadLibrary(L"ntdll.dll");
    NtQueryKey = (NTQK)GetProcAddress(GetModuleHandle(L"ntdll.dll"), "NtQueryKey");
    if (!AdjustCurrentPrivilege(SE_TAKE_OWNERSHIP_NAME)) {
        return 1;
    }

    // =====================================================================
    // SAFE HWID SPOOFING ONLY
    // This function ONLY changes hardware identifiers in the registry.
    // It does NOT delete any user files, system files, or game files.
    // It does NOT modify volume boot sectors.
    // It does NOT delete system restore points.
    // =====================================================================

    // --- Spoof Monitor EDID REMOVED ---
    // Completely randomizing EDID breaks its 8-byte validation header and structured
    // descriptors. DXGI, WMI, or display drivers crash with heap corruption when
    // attempting to parse the malformed EDID.

    // --- Spoof Motherboard Hardware Config GUID ---
    SpoofUniqueThen(HKEY_LOCAL_MACHINE, L"SYSTEM\\HardwareConfig", L"LastConfig", {
        ForEachSubkey(key, {
            if (_wcsicmp(name, L"current")) {
                RenameSubkey(key, name, spoof);
                break;
            }
        });
    });

    // --- Spoof NVIDIA GPU identifiers ---
    SpoofUnique(HKEY_LOCAL_MACHINE, L"SOFTWARE\\NVIDIA Corporation\\Global", L"ClientUUID");
    SpoofUnique(HKEY_LOCAL_MACHINE, L"SOFTWARE\\NVIDIA Corporation\\Global", L"PersistenceIdentifier");
    SpoofUnique(HKEY_LOCAL_MACHINE, L"SOFTWARE\\NVIDIA Corporation\\Global\\CoProcManager", L"ChipsetMatchID");

    // --- Spoof various machine identifiers ---
    SpoofBinary(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Services\\TPM\\WMI", L"WindowsAIKHash");
    SpoofBinary(HKEY_CURRENT_USER, L"Software\\Microsoft\\Direct3D", L"WHQLClass");
    SpoofBinary(HKEY_CURRENT_USER, L"Software\\Classes\\Installer\\Dependencies", L"MSICache");

    // --- Spoof Disk identifiers ---
    OpenThen(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\MultifunctionAdapter\\0\\DiskController\\0\\DiskPeripheral", {
        ForEachSubkey(key, {
            SpoofUnique(key, name, L"Identifier");
        });
    });

    // --- Spoof SCSI identifiers ---
    OpenThen(HKEY_LOCAL_MACHINE, L"HARDWARE\\DEVICEMAP\\Scsi", {
        ForEachSubkey(key, {
            OpenThen(key, name, {
                ForEachSubkey(key, {
                    OpenThen(key, name, {
                        ForEachSubkey(key, {
                            if (wcsstr(name, L"arget")) {
                                OpenThen(key, name, {
                                    ForEachSubkey(key, {
                                        SpoofUnique(key, name, L"Identifier");
                                    });
                                });
                            }
                        });
                    });
                });
            });
        });
    });

    // --- Spoof TPM random seed ---
    SpoofBinary(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Services\\TPM\\ODUID", L"RandomSeed");

    // --- Spoof Machine GUID (main identifier used by anti-cheats including FiveM) ---
    SpoofUnique(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Cryptography", L"MachineGuid");

    // --- Spoof Hardware Profile GUID ---
    SpoofUnique(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\IDConfigDB\\Hardware Profiles\\0001", L"HwProfileGuid");

    // --- Spoof Windows Update tracking IDs ---
    SpoofUnique(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\WindowsUpdate", L"AccountDomainSid");
    SpoofUnique(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\WindowsUpdate", L"PingID");
    SpoofUnique(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\WindowsUpdate", L"SusClientId");
    SpoofBinary(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\WindowsUpdate", L"SusClientIdValidation");

    // --- Spoof network adapter timestamps ---
    SpoofBinary(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Services\\Tcpip6\\Parameters", L"Dhcpv6DUID");

    // --- Spoof Computer Hardware ID ---
    SpoofUnique(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\SystemInformation", L"ComputerHardwareId");
    SpoofUniques(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\SystemInformation", L"ComputerHardwareIds");

    // --- Spoof IE Migration date ---
    SpoofBinary(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Internet Explorer\\Migration", L"IE Installed Date");

    // --- Spoof SQMClient Machine ID ---
    SpoofUnique(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\SQMClient", L"MachineId");
    SpoofQWORD(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\SQMClient", L"WinSqmFirstSessionStartTime");

    // --- Spoof Install timestamps ---
    SpoofQWORD(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", L"InstallTime");
    SpoofQWORD(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", L"InstallDate");

    // --- Spoof Digital Product IDs ---
    SpoofBinary(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", L"DigitalProductId");
    SpoofBinary(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", L"DigitalProductId4");

    // --- Spoof Build info ---
    SpoofUnique(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", L"BuildGUID");
    SpoofUnique(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", L"ProductId");
    SpoofUnique(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", L"BuildLab");
    SpoofUnique(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", L"BuildLabEx");

    // --- Spoof GPU driver identifiers ---
    SpoofUnique(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0000", L"_DriverProviderInfo");
    SpoofUnique(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0000", L"UserModeDriverGUID");

    // --- Spoof Network adapter install timestamps (safe - only changes timestamps) ---
    OpenThen(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e972-e325-11ce-bfc1-08002be10318}", {
        ForEachSubkey(key, {
            if (_wcsicmp(name, L"configuration") && _wcsicmp(name, L"properties")) {
                SpoofQWORD(key, name, L"NetworkInterfaceInstallTimestamp");
            }
        });
    });

    // --- Spoof DiagTrack timestamps ---
    SpoofQWORD(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Diagnostics\\DiagTrack\\SevilleEventlogManager", L"LastEventlogWrittenTime");
    SpoofQWORD(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\SoftwareProtectionPlatform\\Activation", L"ProductActivationTime");

    // --- Spoof UEFI/ESRT identifiers ---
    OpenThen(HKEY_LOCAL_MACHINE, L"HARDWARE\\UEFI\\ESRT", {
        WCHAR subkeys[0xFF][MAX_PATH] = { 0 };
        DWORD subkeys_length = 0;

        ForEachSubkey(key, {
            wcscpy(subkeys[subkeys_length++], name);
        });

        for (DWORD i = 0; i < subkeys_length; ++i) {
            WCHAR spoof[MAX_PATH] = { 0 };
            wcscpy(spoof, subkeys[i]);
            OutSpoofUnique(spoof);
            RenameSubkey(key, subkeys[i], spoof);
        }
    });

    // --- Spoof Windows Defender Reporting MachineId ---
    SpoofUnique(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows Defender\\Reporting", L"MachineId");

    // =====================================================================
    // FiveM-SPECIFIC REGISTRY SPOOFING (Native API)
    // These target identifiers FiveM's anti-cheat specifically reads.
    // =====================================================================

    // --- Spoof IDE Disk enumeration (FiveM reads disk serial numbers) ---
    OpenThen(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Enum\\IDE", {
        ForEachSubkey(key, {
            OpenThen(key, name, {
                ForEachSubkey(key, {
                    SpoofUnique(key, name, L"FriendlyName");
                });
            });
        });
    });

    // --- Spoof SCSI Disk enumeration ---
    OpenThen(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Enum\\SCSI", {
        ForEachSubkey(key, {
            OpenThen(key, name, {
                ForEachSubkey(key, {
                    SpoofUnique(key, name, L"FriendlyName");
                });
            });
        });
    });

    // --- Spoof Disk Service Enum entries ---
    SpoofUnique(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Services\\Disk\\Enum", L"0");
    SpoofUnique(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Services\\Disk\\Enum", L"1");

    // --- Spoof Network adapter MAC addresses (NetworkAddress) ---
    OpenThen(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e972-e325-11ce-bfc1-08002be10318}", {
        ForEachSubkey(key, {
            if (_wcsicmp(name, L"configuration") && _wcsicmp(name, L"properties")) {
                SpoofUnique(key, name, L"NetworkAddress");
            }
        });
    });

    // --- SMBiosData spoofing REMOVED ---
    // The SMBiosData value is a structured SMBIOS table with headers, checksums,
    // and string tables. Overwriting it with random bytes corrupts the structure
    // and causes heap corruption when FiveM/WMI parses it.
    // FiveM BIOS identifiers are already spoofed individually above
    // (SystemManufacturer, SystemProductName, BIOSVendor, etc.).

    // --- Spoof BIOS information values (additional FiveM fingerprints) ---
    SpoofUnique(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\BIOS", L"SystemManufacturer");
    SpoofUnique(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\BIOS", L"SystemProductName");
    SpoofUnique(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\BIOS", L"BIOSVendor");
    SpoofUnique(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\BIOS", L"BIOSVersion");
    SpoofUnique(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\BIOS", L"BaseBoardManufacturer");
    SpoofUnique(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\BIOS", L"BaseBoardVersion");
    SpoofUnique(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\BIOS", L"BaseBoardProduct");
    SpoofUnique(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\BIOS", L"SystemFamily");
    SpoofUnique(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\BIOS", L"SystemVersion");
    SpoofUnique(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\BIOS", L"SystemSKU");

    // --- Spoof Computer Name (FiveM can read hostname) ---
    SpoofUnique(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\ComputerName\\ActiveComputerName", L"ComputerName");
    SpoofUnique(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\ComputerName\\ComputerName", L"ComputerName");

    // --- Spoof Volume Serial Number via MountedDevices (CRITICAL for FiveM) ---
    SpoofBinary(HKEY_LOCAL_MACHINE, L"SYSTEM\\MountedDevices", L"\\DosDevices\\C:");
    SpoofBinary(HKEY_LOCAL_MACHINE, L"SYSTEM\\MountedDevices", L"\\DosDevices\\D:");

    // --- Spoof TCP/IP hostname (must match spoofed ComputerName) ---
    SpoofUnique(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters", L"Hostname");
    SpoofUnique(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters", L"NV Hostname");

    // --- Spoof DHCP identifiers ---
    SpoofBinary(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Services\\Tcpip6\\Parameters", L"Dhcpv6DUID");

    // NOTE: The following dangerous operations have been REMOVED:
    // - NO file/folder deletion (no ForceDeleteFile, no rmdir, no DeleteFileW)
    // - NO NTUSER.DAT deletion (this was causing lock screen user loss)
    // - NO System Restore point deletion (vssadmin delete shadows)
    // - NO NTFS journal deletion (fsutil usn deletejournal)
    // - NO SMBIOS data corruption
    // - NO temp file wiping
    // - NO desktop.ini recursive deletion

    return 0;
}
