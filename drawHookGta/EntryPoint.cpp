#define _CRT_SECURE_NO_WARNINGS

#include "Common.hpp"
#include "MinHook/include/MinHook.h"

using namespace Main;

typedef HRESULT(__stdcall* d3d11_present_t)(IDXGISwapChain* p_swap_chain, UINT sync_interval, UINT flags);
d3d11_present_t o_d3d11_present = nullptr;

typedef LRESULT(CALLBACK* wndProc_t) (HWND, UINT, WPARAM, LPARAM);
wndProc_t o_wndProc = nullptr;

std::once_flag init_d3d;

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK hooked_wndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_KEYDOWN)
	{
		switch (wParam)
		{
			case VK_INSERT: g_bDrawMenu = !g_bDrawMenu;
		}
	}

	ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);
	return CallWindowProc(o_wndProc, hWnd, uMsg, wParam, lParam);
}

HRESULT hooked_d3d11_present(IDXGISwapChain* p_swap_chain, UINT sync_interval, UINT flags) {
	std::call_once(init_d3d, [&] {
		p_swap_chain->GetDevice(__uuidof(m_pDevice), reinterpret_cast<void**>(&m_pDevice));
		m_pDevice->GetImmediateContext(&m_pContext);

		DXGI_SWAP_CHAIN_DESC sd;
		p_swap_chain->GetDesc(&sd);
		g_hWnd = sd.OutputWindow;

		ID3D11Texture2D* pBackBuffer = NULL;
		p_swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
		m_pDevice->CreateRenderTargetView(pBackBuffer, NULL, &m_pRenderTargetView);
		pBackBuffer->Release();

		o_wndProc = (WNDPROC)SetWindowLongPtr(g_hWnd, GWLP_WNDPROC, (LONG_PTR)hooked_wndProc);

		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags = ImGuiConfigFlags_NoMouseCursorChange;
		ImGui_ImplWin32_Init(g_hWnd);
		ImGui_ImplDX11_Init(m_pDevice, m_pContext);

		});

	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	if (g_bDrawMenu)
	{
		ImGui::Begin("ImGui Window");
		ImGui::Text("Test ImGUI Window");
		ImGui::End();
	}

	ImGui::EndFrame();
	ImGui::Render();

	m_pContext->OMSetRenderTargets(1, &m_pRenderTargetView, NULL);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	return o_d3d11_present(p_swap_chain, sync_interval, flags);
}


BOOL APIENTRY DllMain(HMODULE hmod, DWORD reason, PVOID)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		DisableThreadLibraryCalls(hmod);

		g_hModule = hmod;
		g_MainThread = CreateThread(nullptr, 0, [](PVOID) -> DWORD
		{
			const auto renderer_handle = reinterpret_cast<uintptr_t>(GetModuleHandleA("GameOverlayRenderer64.dll"));
			const auto function_to_hook = renderer_handle + 0x8CF40;
			
			if (MH_Initialize() != MH_OK)
			{
				printf("Failed to initialize MinHook");
				return FALSE;
			}

			MH_STATUS hookStatus = MH_CreateHook(reinterpret_cast<LPVOID>(function_to_hook), &hooked_d3d11_present, reinterpret_cast<LPVOID*>(&o_d3d11_present));

			if (hookStatus != MH_OK)
			{
				printf("Failed to create hook for D3D11Present : %s", MH_StatusToString(hookStatus));
				return FALSE;
			}

			hookStatus = MH_EnableHook(reinterpret_cast<LPVOID>(function_to_hook));
			if (hookStatus != MH_OK)
			{
				printf("Failed to enable hook for D3D11Present : %s", MH_StatusToString(hookStatus));
				return FALSE;
			}

			while (!GetAsyncKeyState(VK_NUMPAD0))
				Sleep(100);

			ImGui_ImplDX11_Shutdown();
			ImGui_ImplWin32_Shutdown();
			ImGui::DestroyContext();

			SetWindowLongPtr(g_hWnd, GWLP_WNDPROC, (LONG_PTR)o_wndProc);

			if (MH_DisableHook(reinterpret_cast<LPVOID>(function_to_hook)) != MH_OK)
			{
				printf("Failed to disable hook for D3D11Present");
				return FALSE;
			}

			if (MH_Uninitialize() != MH_OK)
			{
				printf("Failed to uninitialize MinHook");
				return FALSE;
			}

			CloseHandle(g_MainThread);
			FreeLibraryAndExitThread(g_hModule, 0);
		}, nullptr, 0, &g_MainThreadId);
	}

	return true;
}
