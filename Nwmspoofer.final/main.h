#pragma once
#pragma warning(disable:4267 4244)
#include <iostream>
#include <windows.h>
#include <thread>

#include <d3d9.h>
#pragma comment(lib,"d3d9.lib")

#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx9.h"
#include "imgui/imgui_impl_win32.h"

// Replaced D3DX SDK dependency with Windows built-in WIC
#include <wincodec.h>
#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "ole32.lib")

#include "bytes/font.h"
#include "bytes/logo.hpp"

#include "encrypt/xor.h"
#include "btfile/bytetofile.hpp"

// Anti-fingerprinting: Use randomized temp file name instead of static hash
// that anti-cheats could detect as a known spoofer signature
#include <random>
static std::string GenerateRandomTempPath() {
    static const char hex[] = "0123456789abcdef";
    std::string name;
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> dist(0, 15);
    for (int i = 0; i < 32; ++i) name += hex[dist(rng)];
    char* tempDir = nullptr;
    size_t len = 0;
    if (_dupenv_s(&tempDir, &len, "TEMP") == 0 && tempDir) {
        std::string result = std::string(tempDir) + "\\" + name + ".png";
        free(tempDir);
        return result;
    }
    return "C:\\Windows\\Temp\\" + name + ".png";
}
std::string imagePath = GenerateRandomTempPath();

#define COLOUR(x) x/255 
#define CENTER(width) ((ImGui::GetWindowSize().x - width) * 0.5f)

bool loader_active = true;
HWND main_hwnd = nullptr;

LPDIRECT3DDEVICE9        g_pd3dDevice;
D3DPRESENT_PARAMETERS    g_d3dpp;
LPDIRECT3D9              g_pD3D;

bool LoadTextureFromFile(const char* filename, PDIRECT3DTEXTURE9* out_texture, int* out_width, int* out_height)
{
    // Use Windows Imaging Component (WIC) instead of D3DX SDK
    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    IWICImagingFactory* pFactory = NULL;
    HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
        IID_IWICImagingFactory, (void**)&pFactory);
    if (FAILED(hr)) return false;

    // Convert filename to wide string
    int len = MultiByteToWideChar(CP_ACP, 0, filename, -1, NULL, 0);
    WCHAR* wFilename = new WCHAR[len];
    MultiByteToWideChar(CP_ACP, 0, filename, -1, wFilename, len);

    IWICBitmapDecoder* pDecoder = NULL;
    hr = pFactory->CreateDecoderFromFilename(wFilename, NULL, GENERIC_READ,
        WICDecodeMetadataCacheOnLoad, &pDecoder);
    delete[] wFilename;
    if (FAILED(hr)) { pFactory->Release(); return false; }

    IWICBitmapFrameDecode* pFrame = NULL;
    hr = pDecoder->GetFrame(0, &pFrame);
    if (FAILED(hr)) { pDecoder->Release(); pFactory->Release(); return false; }

    IWICFormatConverter* pConverter = NULL;
    hr = pFactory->CreateFormatConverter(&pConverter);
    if (FAILED(hr)) { pFrame->Release(); pDecoder->Release(); pFactory->Release(); return false; }

    hr = pConverter->Initialize(pFrame, GUID_WICPixelFormat32bppBGRA, WICBitmapDitherTypeNone,
        NULL, 0.0f, WICBitmapPaletteTypeCustom);
    if (FAILED(hr)) { pConverter->Release(); pFrame->Release(); pDecoder->Release(); pFactory->Release(); return false; }

    UINT width = 0, height = 0;
    pConverter->GetSize(&width, &height);

    BYTE* pixels = new BYTE[width * height * 4];
    hr = pConverter->CopyPixels(NULL, width * 4, width * height * 4, pixels);
    if (FAILED(hr)) {
        delete[] pixels;
        pConverter->Release(); pFrame->Release(); pDecoder->Release(); pFactory->Release();
        return false;
    }

    // Create D3D9 texture
    PDIRECT3DTEXTURE9 texture = NULL;
    hr = g_pd3dDevice->CreateTexture(width, height, 1, D3DUSAGE_DYNAMIC, D3DFMT_A8R8G8B8,
        D3DPOOL_DEFAULT, &texture, NULL);
    if (FAILED(hr)) {
        delete[] pixels;
        pConverter->Release(); pFrame->Release(); pDecoder->Release(); pFactory->Release();
        return false;
    }

    D3DLOCKED_RECT rect;
    hr = texture->LockRect(0, &rect, NULL, D3DLOCK_DISCARD);
    if (SUCCEEDED(hr)) {
        for (UINT y = 0; y < height; y++) {
            memcpy((BYTE*)rect.pBits + y * rect.Pitch, pixels + y * width * 4, width * 4);
        }
        texture->UnlockRect(0);
    }

    delete[] pixels;
    pConverter->Release();
    pFrame->Release();
    pDecoder->Release();
    pFactory->Release();

    *out_texture = texture;
    *out_width = (int)width;
    *out_height = (int)height;
    return true;
}

inline bool FileExists(const std::string& name) {
    struct stat buffer;
    return (stat(name.c_str(), &buffer) == 0);
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

void Theme() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.Colors[ImGuiCol_WindowBg] = ImVec4(COLOUR(22.0f), COLOUR(24.0f), COLOUR(29.0f), 1.f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(COLOUR(30.0f), COLOUR(31.0f), COLOUR(38.0f), 1.f);
    style.Colors[ImGuiCol_Border] = ImVec4(COLOUR(22.0f), COLOUR(24.0f), COLOUR(29.0f), 0.9f);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(COLOUR(50.0f), COLOUR(50.0f), COLOUR(50.0f), 1.f);
    style.Colors[ImGuiCol_Button] = ImVec4(COLOUR(67.0f), COLOUR(38.0f), COLOUR(235.0f), 1.f);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(COLOUR(5.0f), COLOUR(116.0f), COLOUR(203.0f), 1.f);
    style.Colors[ImGuiCol_Header] = ImVec4(COLOUR(5.0f), COLOUR(116.0f), COLOUR(203.0f), 1.f);
    style.WindowRounding = 0.0f;
    style.FrameRounding = 2.f;
}

bool CreateDeviceD3D(HWND hWnd)
{
    if ((g_pD3D = Direct3DCreate9(D3D_SDK_VERSION)) == NULL)
        return false;

    // Create the D3DDevice
    ZeroMemory(&g_d3dpp, sizeof(g_d3dpp));
    g_d3dpp.Windowed = TRUE;
    g_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    g_d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
    g_d3dpp.EnableAutoDepthStencil = TRUE;
    g_d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
    g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;           // Present with vsync
    //g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;   // Present without vsync, maximum unthrottled framerate

    // Try hardware vertex processing first, fallback to software (some Win11 setups need this)
    if (g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &g_d3dpp, &g_pd3dDevice) < 0) {
        // Fallback to software vertex processing for compatibility
        if (g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &g_d3dpp, &g_pd3dDevice) < 0) {
            return false;
        }
    }
    return true;
}

void CleanupDeviceD3D()
{
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
    if (g_pD3D) { g_pD3D->Release(); g_pD3D = NULL; }
}

void ResetDevice()
{
    ImGui_ImplDX9_InvalidateDeviceObjects();
    HRESULT hr = g_pd3dDevice->Reset(&g_d3dpp);
    if (hr == D3DERR_INVALIDCALL)
        IM_ASSERT(0);
    ImGui_ImplDX9_CreateDeviceObjects();
}