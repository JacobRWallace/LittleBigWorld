#ifndef UI_H
#define UI_H

#include "../libs/imgui/imgui.h"
#include "../libs/imgui/backends/imgui_impl_glfw.h"
#include "../libs/imgui/backends/imgui_impl_opengl3.h"
#include "object_manager.h"
#include "spawn_manager.h"
#include <map>
#include <glm/glm.hpp>
#include "../libs/stb_image.h"

// ImGui initialization and rendering
class ImGuiManager
{
public:
    static bool Initialize(GLFWwindow* window, const char* glsl_version)
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

        ImGui::StyleColorsDark();

        if (!ImGui_ImplGlfw_InitForOpenGL(window, true))
            return false;

        if (!ImGui_ImplOpenGL3_Init(glsl_version))
            return false;

        return true;
    }

    static void Shutdown()
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

    static void NewFrame()
    {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    static void Render()
    {
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }
};

// Spawn palette UI
class SpawnUI
{
private:
    ObjectManager* objectManager;
    SpawnManager* spawnManager;

    std::map<int, GLuint> iconTextures;
    bool showUI = true;

    static constexpr int ITEMS_PER_ROW = 3;
    static constexpr float ITEM_SIZE = 80.0f;

public:
    SpawnUI(ObjectManager* objMgr, SpawnManager* spawnMgr)
        : objectManager(objMgr), spawnManager(spawnMgr) {}

    ~SpawnUI()
    {
        for (auto& pair : iconTextures)
        {
            glDeleteTextures(1, &pair.second);
        }
    }

    void LoadIconTextures()
    {
        const auto& paletteOrder = objectManager->GetPaletteOrder();
        for (const auto& name : paletteOrder)
        {
            const ObjectDef* def = objectManager->Get(name);
            if (!def) continue;

            // Try to load PNG icon
            std::string iconPath = "assets/icons/" + def->name + ".png";
            GLuint texture = LoadTextureFromFile(iconPath.c_str());

            // If loading failed, try placeholder.png
            if (texture == 0)
            {
                texture = LoadTextureFromFile("assets/icons/placeholder.png");
            }

            // If still failed, create fallback colored icon
            if (texture == 0)
            {
                texture = CreateColoredIcon(def->id, def->visuals.baseColor);
            }

            iconTextures[def->id] = texture;
        }
    }

private:
    GLuint LoadTextureFromFile(const char* filename)
    {
        int width, height, channels;
        unsigned char* data = stbi_load(filename, &width, &height, &channels, 4);

        if (!data)
            return 0;

        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
        return texture;
    }

    GLuint CreateColoredIcon(int id, glm::vec3 color)
    {
        // Create a 64x64 textured icon with the object's base color
        unsigned char* pixels = new unsigned char[64 * 64 * 4];

        for (int y = 0; y < 64; y++)
        {
            for (int x = 0; x < 64; x++)
            {
                int idx = (y * 64 + x) * 4;

                // Create a simple gradient border effect
                int dx = (x < 32) ? x : (64 - x);
                int dy = (y < 32) ? y : (64 - y);
                float dist = glm::sqrt(dx * dx + dy * dy) / 32.0f;

                float intensity = glm::clamp(1.0f - dist * 0.5f, 0.5f, 1.0f);

                pixels[idx + 0] = (unsigned char)(color.r * 255 * intensity);
                pixels[idx + 1] = (unsigned char)(color.g * 255 * intensity);
                pixels[idx + 2] = (unsigned char)(color.b * 255 * intensity);
                pixels[idx + 3] = 255;
            }
        }

        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 64, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        delete[] pixels;
        return texture;
    }

public:

    void Render()
    {
        if (!showUI) return;

        ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - 350, 10), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(340, ImGui::GetIO().DisplaySize.y - 20), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("Spawn Palette", &showUI))
        {
            // Status bar
            ImGui::Text("Mode: %s", spawnManager->IsSpawning() ? "SPAWNING" : "NORMAL");
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.8f, 1.0f), "Total: %d", spawnManager->GetSpawnedCount());

            if (spawnManager->IsSpawning())
            {
                const ObjectDef* selected = objectManager->GetSelectedPaletteObject();
                if (selected)
                {
                    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Selected: %s", selected->displayName.c_str());
                    ImGui::TextWrapped("Click in world to spawn (ESC to cancel)");
                }
            }

            ImGui::Separator();
            ImGui::Text("Objects (Press 0-9):");
            ImGui::Separator();

            // Grid layout for icons
            const auto& paletteOrder = objectManager->GetPaletteOrder();
            float panelWidth = ImGui::GetContentRegionAvail().x;
            float itemWidth = ITEM_SIZE;
            int itemsPerRow = glm::max(1, (int)((panelWidth - 10) / (itemWidth + 10)));

            int col = 0;
            for (int i = 0; i < (int)paletteOrder.size() && i < 10; i++)
            {
                const ObjectDef* def = objectManager->Get(paletteOrder[i]);
                if (!def) continue;

                GLuint iconTex = iconTextures.count(def->id) ? iconTextures[def->id] : 0;

                ImGui::PushID(i);

                bool isSelected = (i == objectManager->GetSelectedPaletteIndex());
                if (isSelected)
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.7f, 1.0f, 0.8f));

                if (ImGui::ImageButton((ImTextureID)(intptr_t)iconTex, ImVec2(ITEM_SIZE, ITEM_SIZE)))
                {
                    objectManager->SelectPaletteItem(i);
                    spawnManager->StartSpawning(def->name);
                }

                if (ImGui::IsItemHovered())
                {
                    ImGui::BeginTooltip();
                    ImGui::Text("[%d] %s (ID: %d)", i, def->displayName.c_str(), def->id);
                    int count = spawnManager->GetSpawnedCountByDefId(def->id);
                    if (count > 0)
                        ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Spawned: %d", count);
                    ImGui::EndTooltip();
                }

                if (isSelected)
                    ImGui::PopStyleColor();

                ImGui::PopID();

                col++;
                if (col < itemsPerRow && i < (int)paletteOrder.size() - 1)
                    ImGui::SameLine();
                else
                    col = 0;
            }

            ImGui::Separator();

            // Spawn stats
            ImGui::Text("Spawned Objects:");
            for (const auto& name : paletteOrder)
            {
                const ObjectDef* def = objectManager->Get(name);
                if (!def) continue;

                int count = spawnManager->GetSpawnedCountByDefId(def->id);
                if (count > 0)
                {
                    ImGui::BulletText("%s: %d", def->displayName.c_str(), count);
                }
            }

            if (spawnManager->IsSpawning() && ImGui::Button("Cancel Spawn (ESC)", ImVec2(-1, 0)))
            {
                spawnManager->CancelSpawning();
            }
        }
        ImGui::End();
    }

    void Toggle() { showUI = !showUI; }
    bool IsVisible() const { return showUI; }
};

#endif
