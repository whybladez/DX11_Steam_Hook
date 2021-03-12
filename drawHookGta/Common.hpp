#pragma once

// WinAPI
#include <Windows.h>
#include <mutex>

// DirectX 11
#include <d3d11.h>

// ImGui
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_win32.h"
#include "ImGui/imgui_impl_dx11.h"

namespace Main
{
	HMODULE g_hModule = nullptr;
	HANDLE g_MainThread = nullptr;
	DWORD g_MainThreadId = NULL;
	HWND g_hWnd = NULL;
	bool g_bDrawMenu = false;

	ID3D11Device* m_pDevice = nullptr;
	ID3D11DeviceContext* m_pContext = nullptr;
	ID3D11RenderTargetView* m_pRenderTargetView = nullptr;
}