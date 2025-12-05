#include <ws2tcpip.h>
#include <Windows.h>
#include <string>
#include <thread>
#include <fstream>
#include <unordered_map>
#include <filesystem>
#include "Utils/Utils.h"
#include "Hook/HookMgr.h"
#include "Logger/Logger.h"
#include <functional>
#include <mutex>
#include "Render/Render.h"
#include "ImGui/imgui.h"
#include "jsoncpp/json.h"
#include "GlobalKeyListener/GlobalKeyListener.h"
#include "IDVGame.h"
#include "Memory/mem.h"
#include <cmath>
#include <iostream>
#include <algorithm>

Logger* logger = nullptr;

class AutoPanelChecker {
private:
    Vector3 local_pos;
    Vector3 butcher_pos;
    Vector3 panel_pos;
    Vector3 panel_dir;
    std::array<float, 2> panel_hit_range;

    const float interact_dis = 26.0f;


public:
    float getCosAngle() {
        auto dir1 = butcher_pos - panel_pos;
        auto dir2 = panel_dir;
        float ll = dir1.magnitude() * dir2.magnitude();
        if (ll < 0.0001f) {
            return 1.0f;
        }
        else {
            return dir1.getCosAngle(dir2);
        }
    }

    AutoPanelChecker(const Vector3& local, const Vector3& butcher, const Vector3& panel,const Vector3& p_dir, std::array<float, 2> panel_hit_range)
        : local_pos(local), butcher_pos(butcher), panel_pos(panel),panel_dir(p_dir),panel_hit_range(panel_hit_range)
        {}

    bool shouldPushPanel() {
        if (local_pos.distanceXZ(panel_pos) > interact_dis) {
            return false;
        }
        if (std::fabs(panel_pos.y - butcher_pos.y) > 20.0f) {
            return false;
        }
        float cos = getCosAngle();
        float dist = panel_pos.distance(butcher_pos);
        float y = std::fabs(dist * cos);
        float x_square = dist * dist - y * y;
        float y_max = panel_hit_range[0];
        return y < y_max && x_square < panel_hit_range[1] * panel_hit_range[1];
    }
};


class IDVCheat {
private:
    GlobalKeyListener key_listener;

    std::vector<std::function<void()>> key_events;
    std::mutex key_events_mtx;
    void addKeyEvent(char key, std::function<void()> handler) {
        key_listener.addKeyEvent(key, [this, handler]() {
            key_events_mtx.lock();
            key_events.push_back(handler);
            key_events_mtx.unlock();
            });
    }
    bool enable_key_events = true;

    bool auto_panel = false;
    std::array<float, 2> panel_hit_range_delta = { 0.5f ,0.0f };

    bool draw_test = false;

    struct DrawInstance {
        std::string check_box;
        IDVGame::ObjectType type;

        bool enable = false;
        ImColor line_color = { 0,255,0 };
        ImColor text_color = { 255,0,0 };
        void renderGui(){
            ImGui::Checkbox(check_box.c_str(), &enable);
        }
        void renderObject(IDVGame::GameObject obj, IDVGame::Matrix const& matrix, GameWindow* window) const {
            if (enable) {
                auto def = obj.getDef();
                if (def->type == type) {
                    
                    if (auto model_object = obj.getModelObject()) {
                        if (auto pos0 = model_object->getPosition()) {
                            if (auto height = model_object->getHeight()) {

                                IDVGame::Position pos = { pos0->x, pos0->y + *height, pos0->z };

                                float screen_pos[2] = { 0.0,0.0 };
                                if (IDVGame::worldToScreen(matrix, window->getWidth(), window->getHeight(), pos, screen_pos)) {
                                    window->drawESPLine(screen_pos[0], screen_pos[1], line_color);
                                    window->drawText(screen_pos[0], screen_pos[1], def->name.c_str(), text_color);
                                }

                                if (auto col_box_ = model_object->getColbox()) {
                                    auto &col_box = *col_box_;

                                    IDVGame::Position vertices[8] = {
                                            {col_box.start.x, col_box.start.y, col_box.start.z},
                                            {col_box.end.x, col_box.start.y, col_box.start.z},
                                            {col_box.end.x, col_box.end.y, col_box.start.z},
                                            {col_box.start.x, col_box.end.y, col_box.start.z},
                                            {col_box.start.x, col_box.start.y, col_box.end.z},
                                            {col_box.end.x, col_box.start.y, col_box.end.z},
                                            {col_box.end.x, col_box.end.y, col_box.end.z},
                                            {col_box.start.x, col_box.end.y, col_box.end.z}
                                    };

                                    float screen_vertices[8][2];
                                    bool vertex_visible[8] = { false };

                                    for (int i = 0; i < 8; i++) {
                                        vertex_visible[i] = IDVGame::worldToScreen(matrix, window->getWidth(), window->getHeight(), vertices[i], screen_vertices[i]);
                                    }

                                    int edges[12][2] = {
                                        {0,1}, {1,2}, {2,3}, {3,0},
                                        {4,5}, {5,6}, {6,7}, {7,4},
                                        {0,4}, {1,5}, {2,6}, {3,7}
                                    };

                                    for (int i = 0; i < 12; i++) {
                                        int v1 = edges[i][0];
                                        int v2 = edges[i][1];

                                        if (vertex_visible[v1] && vertex_visible[v2]) {
                                            window->drawLine(ImVec2(screen_vertices[v1][0], screen_vertices[v1][1]), ImVec2(screen_vertices[v2][0], screen_vertices[v2][1]), line_color);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    };

    std::vector<DrawInstance> draw_list = { 
        {"绘制求生",IDVGame::CIVILIAN},{"绘制监管",IDVGame::BUTCHER},{"绘制椅子",IDVGame::HOOK},{"绘制木板",IDVGame::PANEL}
    };


    ImColor line_color = { 0,255,0 };
    ImColor text_color = { 255,0,0 };

    Memory::MemoryPointer base_addr = Memory::MemoryPointer(0);
    std::vector<IDVGame::GameObject> game_objects;
    std::optional<IDVGame::Matrix> matrix = std::nullopt;
    std::optional<IDVGame::Position> local_pos = std::nullopt;

    IDVGame::GameObject const* getLocalObject() {
        if (!local_pos)return nullptr;
        for (auto const& i : game_objects) {
            if (auto model_object = i.getModelObject()) {
                if (auto pos = model_object->getPosition()) {
                    float distance = pos->distanceXZ(*local_pos);
                    if (distance < 0.001f) {
                        return &i;
                    }
                }
            }
        }
        return nullptr;
    }
public:
    
    void initialize() {
        if (key_listener.startListening() == false) {
            logger->log("key_listener.startListening() == false");
            exit(0);
        }
    }
    void renderUI() {
        ImGuiWindowClass noAutoMerge;
        noAutoMerge.ViewportFlagsOverrideSet = ImGuiViewportFlags_NoAutoMerge;
        ImGui::SetNextWindowClass(&noAutoMerge);
        ImGui::Begin("Wendy");
       
        if (ImGui::CollapsingHeader("CPP层调试信息")) {
            ImGui::Text("neox_engine.dll:%llx", base_addr.get());
            
            if (local_pos) {
                ImGui::Text("自身坐标:%.3f %.3f %.3f", local_pos->x, local_pos->y, local_pos->z);
            }

            auto panel = findNearestObject(IDVGame::PANEL);
            if (panel) {
                ImGui::Text("最近木板距离:%.3f", panel->distance);
            }

            ImGui::Text("数组大小:%d",game_objects.size());
            if (ImGui::Button("DumpHash")) {
                uint32_t index = 0;
                for (auto p : game_objects) {
                    char buf[1024] = { 0 };
                    auto hash = p.getHash();
                    auto identifier = p.getIdentifier();
                    if (hash && identifier) {
                        logger->log("index:%d hash:%llx identifier:%s", index, *hash, identifier->c_str());
                    }
                    index++;
                }
            }
            ImGui::Checkbox("绘制测试",&draw_test);

        }

        ImGui::Checkbox("快捷键", &enable_key_events);
        for (auto& i : draw_list) {
            i.renderGui();
        }

        ImGui::Checkbox("自动砸板", &auto_panel);
        ImGui::SliderFloat("砸板X范围差值", &panel_hit_range_delta[1], 0.0f,3.0f);
        ImGui::SliderFloat("砸板Y范围差值", &panel_hit_range_delta[0], 0.0f,4.0f);

        ImGui::End();
    }
    void tickEvents() {
        key_events_mtx.lock();
        if (enable_key_events) {
            for (auto i : key_events) {
                i();
            }
        }
        key_events.clear();
        key_events_mtx.unlock();
    }

    void renderGameObjects(GameWindow* window) {
        if (matrix) {
            uint32_t index = 0;
            for (auto const& p : game_objects) {

                if (draw_test) {
                    if (auto model_object = p.getModelObject()) {
                        float screen_pos[2] = { 0.0,0.0 };
                        auto pos = model_object->getPosition();
                        if (pos && IDVGame::worldToScreen(*matrix, window->getWidth(), window->getHeight(), *pos, screen_pos)) {
                            window->drawESPLine(screen_pos[0], screen_pos[1], line_color);
                            char t[1024] = { 0 };

                            auto hash = p.getHash();
                            auto height = model_object->getHeight();
                            auto identifier = p.getIdentifier();
                            auto len = p.getHashedLength();

                            if (hash && height && identifier && len) {
                                sprintf(t, "hash:%llx index:%d height:%.3f name:%s len:%llu", *hash, index, *height, identifier->c_str(),*len);
                                window->drawText(screen_pos[0], screen_pos[1], t, text_color);
                            }
                        }
                    }
                }
                for (auto const& i : draw_list) {
                    i.renderObject(p, *matrix, window);
                }

                index += 1;
            }
        }
    }

    struct ObjectResult {
        float distance;
        IDVGame::GameObject const* obj;
    };


    std::optional<ObjectResult> findNearestObject(IDVGame::ObjectType type) {
        if (!local_pos)return std::nullopt;
        IDVGame::GameObject const* nearest_obj = nullptr;
        float nearest_dist = 0.0f;
        for (auto const& i : game_objects) {
            auto pos = i.getModelObject()->getPosition();
            if (!pos) {
                continue;
            }
            float distance = local_pos->distance(*pos);
            if (i.getDef()->type == type) {
                if (nearest_obj == nullptr) {
                    nearest_obj = &i;
                    nearest_dist = distance;
                }
                else {
                    if (distance < nearest_dist) {
                        nearest_dist = distance;
                        nearest_obj = &i;
                    }
                }
            }          
        }
        if (nearest_obj) {
            return  ObjectResult{nearest_dist,nearest_obj };
        }
        else {
            return std::nullopt;
        }
    }

    void pushPanel(GameWindow* window) {
        SetForegroundWindow(window->getHwnd());
        keybd_event(VK_SPACE, 0, 0, 0);
        keybd_event(VK_SPACE, 0, KEYEVENTF_KEYUP, 0);
    }

    void tickAutoPanel(GameWindow* window) {
        if (auto_panel == false)return;
        if (!local_pos)return;
        auto butcher = findNearestObject(IDVGame::BUTCHER);
        if (!butcher)return;
        auto panel = findNearestObject(IDVGame::PANEL);
        if (!panel)return;
        auto panel_model = panel->obj->getModelObject();
        if (!panel_model)return;
        auto panel_height = panel_model->getHeight();
        if (!panel_height || *panel_height < 20.0f)return;
        auto panel_pos = panel_model->getPosition();
        auto panel_dir = panel_model->getDirection();
        if (!panel_pos || !panel_dir)return;
        auto butcher_model = butcher->obj->getModelObject();
        if (!butcher_model)return;
        auto butcher_pos = butcher_model->getPosition();
        if (!butcher_pos)return;

        AutoPanelChecker checker = AutoPanelChecker(*local_pos, *butcher_pos, *panel_pos, *panel_dir, {13.0f - panel_hit_range_delta[1],12.0f - panel_hit_range_delta[0]});
        if (checker.shouldPushPanel()) {
            pushPanel(window);
        }
    }

    void prepare() {
        base_addr = Memory::get_module_base(L"neox_engine.dll");
        matrix = IDVGame::readMatrix(base_addr);
        game_objects = IDVGame::readObjectArray(base_addr);
        local_pos = IDVGame::readLocalPosition(base_addr);
    }

    void destroy() {
        matrix = std::nullopt;
        local_pos = std::nullopt;
    }

    void tick(GameWindow* window) {
        if (Memory::attach_process() && window) {
            prepare();
            tickEvents();
            tickAutoPanel(window);
            renderGameObjects(window);
            renderUI();
            destroy();
            Memory::detach_process();
        }
    }
};

void initCheatUI() {
    Render* render = Render::getInstance();
    IDVCheat* cheat = new IDVCheat;
    cheat->initialize();
    render->setup(L"第五人格", logger, [=](GameWindow* window) {
        cheat->tick(window);
    });
    render->start();
}

void initLogger() {
    logger = Logger::createInstance("F:\\neox_template\\read\\wendy_log_memory.txt", true);
    logger->log("entry loaded");
}

void wait() {
    while (true) {
        Sleep(1000000);
    }
}

void initMemory() {
    if (Memory::init(L"dwrg.exe") == false) {
        logger->log("memory module initialize failed");
        wait();
    }
}

void entry() {
    initMemory();
    initLogger();
    initCheatUI();
}

int main() {
    entry();
    wait();
}
BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
)
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        entry();
    }
    return TRUE;
}

