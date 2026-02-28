#pragma once
#include <iostream>
#include <windows.h>
#include <thread>
#include <random>
#include <d3d9.h>
#pragma comment(lib,"d3d9.lib")

#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx9.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_internal.h"

// Anti-fingerprinting: Generate a random window class name at startup
// so anti-cheats can't detect the spoofer by scanning for known window titles
static std::string GenerateRandomWindowClass() {
    static const char alphanum[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    std::string result = "Win_";
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> dist(0, sizeof(alphanum) - 2);
    for (int i = 0; i < 8; ++i) result += alphanum[dist(rng)];
    return result;
}
static std::string _randWindowTitle = GenerateRandomWindowClass();
#define WINDOW_TITLE _randWindowTitle.c_str()
#define WINDOW_WIDTH  750
#define WINDOW_HEIGHT 450
#define COLOUR(x) x/255

enum states {
	loading,
	menu
};

class Menu {
public:
	ImFont* smallFont;
	ImFont* mediumFont;
	void Main(bool loader_active, PDIRECT3DTEXTURE9 my_texture);
private:
	void MainMenu();
	void LoadingScene();
};

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

extern std::unique_ptr<Menu> m_Menu;