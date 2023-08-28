#include "overlay.h"
#include "ImGui\customed.h"
#include "Utils\NotSDK.h"

// Set your ID
char LocalName[32] = "YOUR SOLDIER NAME";

// LocalPlayer Data
uint64_t LocalPlayer = 0;
uint64_t LocalSoldier = 0;
int LocalTeam = 0;
float LocalHealth = 0;
Vector3 LocalPosition = Vector3(0.f, 0.f, 0.f);

void Overlay::m_Info()
{
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2((float)GameRect.right, (float)GameRect.bottom));
    ImGui::Begin("##Info", (bool*)NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoInputs);

    ImGui::Text("Battlefield 1 External [%.1f FPS]", ImGui::GetIO().Framerate);
    // Time
    time_t t = time(nullptr);
    struct tm nw;
    errno_t nTime = localtime_s(&nw, &t);
    ImGui::Text("%d:%d:%d", nw.tm_hour, nw.tm_min, nw.tm_sec);

    ImGui::End();
}

void Overlay::m_Menu()
{
    ImGui::Begin("Battlefield 1 [ EXTERNAL ]", &ShowMenu, ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::Toggle("ESP", &g.ESP, g.ESP);

    ImGui::SeparatorText("ESP Options");

    ImGui::Checkbox("Box", &g.vBox);
    ImGui::Checkbox("Line", &g.vLine);
    ImGui::Checkbox("HealthBar", &g.vHealth);
    ImGui::Checkbox("Distance", &g.vDistance);
    ImGui::Checkbox("Name", &g.vName);

    ImGui::NewLine();

    ImGui::InputText("SoldierName", LocalName, sizeof(LocalName));
    ImGui::End();
}

// Player Only
void Overlay::m_ESP()
{
    // ImGui Window
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2((float)GameRect.right, (float)GameRect.bottom));
    ImGui::Begin("##ESP", (bool*)NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoInputs);

    for (int i = 0; i < 64; i++)
    {
        // Get Player
        uint64_t Player = GetPlayerById(i);

        if (Player == NULL)
            continue;

        // Get Soldier
        uint64_t clientSoldierEntity = m.Read<uint64_t>(Player + 0x1D48);

        if (!clientSoldierEntity)
            continue;

        // Get PlayerName & Get LocalPlayer "By NAME" <------- ”ñí‚Éd—v
        char pName[32];
        ReadProcessMemory(m.pHandle, (void*)(Player + offset::PlayerName), &pName, sizeof(pName), NULL);

        // if PlayerName == Input Name
        if (strcmp(pName, LocalName) == 0)
        {
            LocalPlayer = Player;
            LocalSoldier = clientSoldierEntity;
            LocalTeam = m.Read<int>(Player + offset::TeamID);
            uint64_t HealthComponent = m.Read<uint64_t>(clientSoldierEntity + 0x1D0);
            LocalHealth = m.Read<float>(HealthComponent + 0x20);
            LocalPosition = m.Read<Vector3>(clientSoldierEntity + offset::location);
        }

        if (Player == LocalPlayer)
            continue;

        // Team
        int Team = m.Read<int>(Player + offset::TeamID);

        // Health
        uint64_t HealthComponent = m.Read<uint64_t>(clientSoldierEntity + 0x1D0);
        float Health = m.Read<float>(HealthComponent + 0x20);

        // Position
        Vector3 Position = m.Read<Vector3>(clientSoldierEntity + offset::location);

        // SomeChecks
        if (Team == LocalTeam)
            continue;
        else if (Health <= 0)
            continue;
        else if (Position == Vector3(0.f, 0.f, 0.f))
            continue;

        // WorldToScreen
        Vector2 ScreenPosition = Vector2(0.f, 0.f);
        WorldToScreen(Position, ScreenPosition);
  
        // Invalid Player
        if (ScreenPosition == Vector2(0.f, 0.f))
            continue;

        // VisCheck
        bool occluded = m.Read<bool>(clientSoldierEntity + offset::occluded);

        // Set ESPColor
        ImColor color = occluded ? ESP_Normal : ESP_Visible;

        // LINE
        if (g.vLine)
            DrawLine(ImVec2(GameRect.right / 2.f, GameRect.bottom), ImVec2(ScreenPosition.x, ScreenPosition.y), color, 1.f);

        // Box—p
        Vector3 BoxTop = Position + GetAABB(clientSoldierEntity).Max;
        Vector3 BoxBottom = Position + GetAABB(clientSoldierEntity).Min;
        Vector2 vTop;
        Vector2 vBom;
        WorldToScreen(BoxTop, vTop);
        WorldToScreen(BoxBottom, vBom);

        float BoxMiddle = ScreenPosition.x;
        float Height = vBom.y - vTop.y;
        float Width = Height / 2.f;

        // Box
        if (g.vBox)
        {
            DrawLine(ImVec2(BoxMiddle + (Width / 2.f), vTop.y), ImVec2(BoxMiddle - (Width / 2.f), vTop.y), color, 1.f);
            DrawLine(ImVec2(BoxMiddle + (Width / 2.f), ScreenPosition.y), ImVec2(BoxMiddle - (Width / 2.f), ScreenPosition.y), color, 1.f);
            DrawLine(ImVec2(BoxMiddle + (Width / 2.f), vTop.y), ImVec2(BoxMiddle + (Width / 2.f), ScreenPosition.y), color, 1.f);
            DrawLine(ImVec2(BoxMiddle - (Width / 2.f), vTop.y), ImVec2(BoxMiddle - (Width / 2.f), ScreenPosition.y), color, 1.f);
        }

        if (g.vHealth)
            HealthBar(BoxMiddle - (Width / 2.f) - 4, ScreenPosition.y, 2.f, -Height, Health, 100.f);

        std::string vContext;

        if (g.vDistance)
        {
            float distance = GetDistance(LocalPosition, Position);
            vContext = std::to_string((int)distance) + "m";
        }

        if (g.vName)
            vContext = vContext + "|" + pName;

        ImVec2 textSize = ImGui::CalcTextSize(vContext.c_str());
        float TextCentor = textSize.x / 2.f;
        String(ImVec2(ScreenPosition.x - TextCentor, ScreenPosition.y), ImColor(1.f, 1.f, 1.f, 1.f), vContext.c_str());
    }

    ImGui::End();
}