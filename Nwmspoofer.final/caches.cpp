#include "caches.h"

void CleanEAC() {
	// Only clean EAC tracking registry (safe - does not delete services)
	system(_xor_("reg delete HKLM\\SOFTWARE\\WOW6432Node\\EasyAntiCheat /f").c_str());
	// REMOVED: Service deletion for EasyAntiCheat and BattlEye services.
	// Deleting these services breaks game anti-cheat reinstallation and
	// causes games (including FiveM) to fail to launch.
}