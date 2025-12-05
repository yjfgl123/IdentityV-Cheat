#include "Render.h"

#include "../Logger/Logger.h"
#include "../Utils/Utils.h"
#include "../ImGui/imgui_impl_win32.h"
#include "../ImGui/imgui_impl_dx11.h"
#include "d3d11helper.h"

Render* g_render = nullptr;

Render* Render::getInstance()
{
	if (g_render == nullptr) {
		g_render = new Render;
	}
    return g_render;
}

void Render::setup(std::wstring const& game_window_name, Logger* logger, std::function<void(GameWindow*)> logic)
{
	this->game_window_name = game_window_name;
	this->logger = logger;
    this->logic = logic;
}

void Render::start()
{
	render_thread = std::thread([=]() {
       SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

		ImGui_ImplWin32_EnableDpiAwareness();
		float main_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));

		std::wstring lpszClassName = Utils::generate_random_wstring(12);
		std::wstring lpWindowName = Utils::generate_random_wstring(12);

		WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, lpszClassName.c_str(), nullptr };
		::RegisterClassExW(&wc);
		HWND hwnd = ::CreateWindowW(wc.lpszClassName, lpWindowName.c_str(), WS_POPUP, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), nullptr, nullptr, wc.hInstance, nullptr);
        SetWindowDisplayAffinity(hwnd, WDA_EXCLUDEFROMCAPTURE);
		if (!CreateDeviceD3D(hwnd))
		{
			CleanupDeviceD3D();
			::UnregisterClassW(wc.lpszClassName, wc.hInstance);
			logger->log("can not create d3d11 device");
			return;
		}

        ::ShowWindow(hwnd, SW_SHOWDEFAULT);
        ::UpdateWindow(hwnd);

        ImGui_ImplWin32_EnableAlphaCompositing(hwnd);

        LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
        exStyle |= WS_EX_TRANSPARENT | WS_EX_LAYERED;
        SetWindowLong(hwnd, GWL_EXSTYLE, exStyle);

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
        io.IniFilename = nullptr;

        io.Fonts->AddFontFromFileTTF(
            "C:/Windows/Fonts/simhei.ttf",
            18.0f,
            nullptr,
            io.Fonts->GetGlyphRangesChineseFull());

        ImGui::StyleColorsDark();

        ImGuiStyle& style = ImGui::GetStyle();
        style.ScaleAllSizes(main_scale);
        style.FontScaleDpi = main_scale;
        io.ConfigDpiScaleFonts = true;
        io.ConfigDpiScaleViewports = true;

        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            style.WindowRounding = 0.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }
        ImGui_ImplWin32_Init(hwnd);
        ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);
        ImVec4 clear_color = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
        bool done = false;
        while (!done)
        {
            MSG msg;
            while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
            {
                ::TranslateMessage(&msg);
                ::DispatchMessage(&msg);
                if (msg.message == WM_QUIT)
                    done = true;
            }
            if (done) {
                break;
            }
            if (g_SwapChainOccluded && g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED)
            {
                ::Sleep(10);
                continue;
            }
            g_SwapChainOccluded = false;

            if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
            {
                CleanupRenderTarget();
                g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
                g_ResizeWidth = g_ResizeHeight = 0;
                CreateRenderTarget();
            }

            ImGui_ImplDX11_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();
            SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

            GameWindow* window = GameWindow::createInstance(game_window_name.c_str());

            logic(window);

            if (window) {
                delete window;
            }

            ImGui::Render();
            const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
            g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
            g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

            if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
            {
                ImGui::UpdatePlatformWindows();
                ImGui::RenderPlatformWindowsDefault();
            }

            HRESULT hr = g_pSwapChain->Present(1, 0);
            g_SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
        }

        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();

        CleanupDeviceD3D();
        ::DestroyWindow(hwnd);
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

	});
	render_thread.detach();
}

GameWindow* GameWindow::createInstance(LPCWSTR name)
{
    HWND hwnd = FindWindow(NULL, name);
    if (hwnd == nullptr) {
        return nullptr;
    }
    RECT rect;
    POINT pt = { 0,0 };
    if (ClientToScreen(hwnd, &pt) == false) {
        return nullptr;
    };
    if (GetClientRect(hwnd, &rect) == false) {
        return nullptr;
    }
    rect = { pt.x,pt.y,pt.x + rect.right,pt.y + rect.bottom };
    int x = rect.left;
    int y = rect.top;
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;
    GameWindow* window = new GameWindow;
    window->hwnd = hwnd;
    window->x = (float)x;
    window->y = (float)y;
    window->width = (float)width;
    window->height = (float)height;
    return window;
}

void GameWindow::drawLine(ImVec2 const& start, ImVec2 const& end, ImColor const& color)
{
    ImVec2 start_ = { x + start.x,y + start.y };
    ImVec2 end_ = { x + end.x,y + end.y };
    ImGui::GetForegroundDrawList()->AddLine(start_, end_, color);
}

void GameWindow::drawCircle(ImVec2 const& center, float radius, ImColor const& color)
{
    ImVec2 center_ = { x + center.x,y + center.y };
    ImGui::GetForegroundDrawList()->AddCircle(center_, radius, color);
}

ImVec2 GameWindow::drawText(float screen_x, float screen_y, const char* text, ImColor const& color)
{
    ImVec2 text_size = ImGui::CalcTextSize(text);
    ImVec2 pos = { x + screen_x - text_size.x / 2 ,y + screen_y - text_size.y / 2 };
    ImGui::GetForegroundDrawList()->AddText(pos, color, text);
    return text_size;
}

void GameWindow::drawESPLine(float screen_x, float screen_y, ImColor const& color)
{
    ImVec2 start = { x + width / 2,y };
    ImVec2 end = { x + screen_x,y + screen_y };
    ImGui::GetForegroundDrawList()->AddLine(start, end, color);
}

void GameWindow::drawESPLine(ImVec2 const& screen_pos, ImColor const& color)
{
    drawESPLine(screen_pos.x, screen_pos.y, color);
}

void GameWindow::drawESPLineWithBox(float screen_x, float screen_y, float line_height, float distance, ImColor const& color)
{
    float screen_y_ = screen_y - (line_height / distance);
    ImVec2 start = { x + width / 2,y };
    ImVec2 end = { x + screen_x,y + screen_y_ };
    ImGui::GetForegroundDrawList()->AddLine(start, end, color);
}

void GameWindow::drawESPBox(float screen_x, float screen_y, ImVec2 const& size, float distance, ImColor const& color)
{
    ImVec2 size_ = {size.x / distance, size.y / distance};

    ImVec2 a = { screen_x - size_.x / 2,screen_y - size_.y };
    ImVec2 b = { screen_x + size_.x / 2,screen_y - size_.y };
    ImVec2 c = { screen_x - size_.x / 2,screen_y };
    ImVec2 d = { screen_x + size_.x / 2,screen_y };

    drawLine(a, b, color);
    drawLine(b, d, color);
    drawLine(d, c, color);
    drawLine(c, a, color);
}

void GameWindow::drawCenterCircle(float radius, ImColor const& color) {
    ImVec2 center = { width / 2, height / 2 };
    drawCircle(center, radius, color);
}