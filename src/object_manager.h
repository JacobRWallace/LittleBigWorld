#ifndef OBJECT_MANAGER_H
#define OBJECT_MANAGER_H

#include "../libs/json.hpp"
#include <glm/glm.hpp>
#include <map>
#include <vector>
#include <string>
#include <filesystem>
#include <fstream>
#include <iostream>

using json = nlohmann::json;
namespace fs = std::filesystem;

// Object definition structures
struct ObjectDef
{
    struct Model { std::string path; glm::vec3 scale = glm::vec3(1.0f); glm::vec3 offset = glm::vec3(0.0f); } model;
    struct Texture { std::string path; } texture;
    struct Hitbox { std::string type; std::vector<glm::vec3> vertices; glm::vec3 dimensions; glm::vec3 offset = glm::vec3(0.0f); } hitbox;
    struct Physics { float mass = 1.0f; float friction = 0.5f; float bounciness = 0.5f; Hitbox hitbox; } physics;
    struct Visuals { glm::vec3 baseColor = glm::vec3(1.0f); glm::vec3 highlightColor = glm::vec3(1.2f); } visuals;
    struct Interaction { bool canBeGrabbed = true; bool canBeRotated = false; int layerIndex = 1; } interaction;

    int id;
    std::string category;
    std::string name;
    std::string displayName;
};

class ObjectManager
{
private:
    std::map<std::string, ObjectDef> templates;
    std::map<std::string, std::vector<std::string>> byCategory;

    // Palette UI state
    std::vector<std::string> paletteOrder;  // Flat list of all objects for palette selection
    int selectedPaletteIndex = -1;

    static glm::vec3 ParseVec3(const json& j, const std::string& key, const glm::vec3& def = glm::vec3(0.0f))
    {
        if (j.contains(key) && j[key].is_array() && j[key].size() >= 3)
            return glm::vec3(j[key][0], j[key][1], j[key][2]);
        return def;
    }

    static glm::vec3 ParseVec3Array(const json& j)
    {
        if (j.is_array() && j.size() >= 3)
            return glm::vec3(j[0], j[1], j[2]);
        return glm::vec3(0.0f);
    }

public:
    bool Load(const std::string& directory)
    {
        try
        {
            if (!fs::exists(directory))
            {
                std::cerr << "Objects directory not found: " << directory << std::endl;
                return false;
            }

            int count = 0;
            for (const auto& entry : fs::directory_iterator(directory))
            {
                if (entry.path().extension() == ".json")
                {
                    std::ifstream file(entry.path());
                    if (!file.is_open()) continue;

                    json j;
                    file >> j;
                    file.close();

                    ObjectDef obj;
                    obj.id = j.value("id", -1);
                    obj.category = j.value("category", "uncategorized");
                    obj.name = j.value("name", "unknown");
                    obj.displayName = j.value("displayName", obj.name);

                    // Model
                    if (j.contains("model"))
                    {
                        auto& m = j["model"];
                        obj.model.path = m.value("path", "");
                        obj.model.scale = ParseVec3(m, "scale", glm::vec3(1.0f));
                        obj.model.offset = ParseVec3(m, "offset", glm::vec3(0.0f));
                    }

                    // Texture
                    if (j.contains("texture"))
                        obj.texture.path = j["texture"].value("path", "");

                    // Physics & Hitbox
                    if (j.contains("physics"))
                    {
                        auto& p = j["physics"];
                        obj.physics.mass = p.value("mass", 1.0f);
                        obj.physics.friction = p.value("friction", 0.5f);
                        obj.physics.bounciness = p.value("bounciness", 0.5f);

                        if (p.contains("hitbox") && !p["hitbox"].is_null())
                        {
                            auto& h = p["hitbox"];
                            obj.physics.hitbox.type = h.value("type", "box");
                            obj.physics.hitbox.offset = ParseVec3(h, "offset", glm::vec3(0.0f));

                            if (obj.physics.hitbox.type == "box")
                                obj.physics.hitbox.dimensions = ParseVec3(h, "dimensions", glm::vec3(1.0f));
                            else if (obj.physics.hitbox.type == "convexHull" && h.contains("vertices"))
                                for (const auto& v : h["vertices"])
                                    obj.physics.hitbox.vertices.push_back(ParseVec3Array(v));
                        }
                        else
                        {
                            // Auto-generate box hitbox from model scale
                            obj.physics.hitbox.type = "box";
                            obj.physics.hitbox.dimensions = ParseVec3(j["model"], "scale", glm::vec3(1.0f)) * 2.0f;
                            obj.physics.hitbox.offset = glm::vec3(0.0f);
                        }
                    }

                    // Visuals
                    if (j.contains("visuals"))
                    {
                        auto& v = j["visuals"];
                        obj.visuals.baseColor = ParseVec3(v, "baseColor", glm::vec3(1.0f));
                        obj.visuals.highlightColor = ParseVec3(v, "highlightColor", glm::vec3(1.2f));
                    }

                    // Interaction
                    if (j.contains("interaction"))
                    {
                        auto& i = j["interaction"];
                        obj.interaction.canBeGrabbed = i.value("canBeGrabbed", true);
                        obj.interaction.canBeRotated = i.value("canBeRotated", false);
                        obj.interaction.layerIndex = i.value("layerIndex", 1);
                    }

                    templates[obj.name] = obj;
                    byCategory[obj.category].push_back(obj.name);
                    count++;
                }
            }

            std::cout << "Loaded " << count << " objects from " << directory << std::endl;
            return count > 0;
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error loading objects: " << e.what() << std::endl;
            return false;
        }
    }

    const ObjectDef* Get(const std::string& name) const
    {
        auto it = templates.find(name);
        return it != templates.end() ? &it->second : nullptr;
    }

    std::vector<std::string> GetCategory(const std::string& cat) const
    {
        auto it = byCategory.find(cat);
        return it != byCategory.end() ? it->second : std::vector<std::string>();
    }

    std::vector<std::string> GetCategories() const
    {
        std::vector<std::string> cats;
        for (const auto& p : byCategory) cats.push_back(p.first);
        return cats;
    }

    int Count() const { return templates.size(); }

    // Palette UI methods
    void BuildPalette()
    {
        paletteOrder.clear();
        for (const auto& cat : GetCategories())
        {
            auto objNames = GetCategory(cat);
            for (const auto& name : objNames)
            {
                paletteOrder.push_back(name);
            }
        }
    }

    bool SelectPaletteItem(int index)
    {
        if (index >= 0 && index < static_cast<int>(paletteOrder.size()))
        {
            selectedPaletteIndex = index;
            return true;
        }
        return false;
    }

    const ObjectDef* GetSelectedPaletteObject() const
    {
        if (selectedPaletteIndex >= 0 && selectedPaletteIndex < static_cast<int>(paletteOrder.size()))
        {
            return Get(paletteOrder[selectedPaletteIndex]);
        }
        return nullptr;
    }

    int GetPaletteItemCount() const
    {
        return paletteOrder.size();
    }

    int GetSelectedPaletteIndex() const
    {
        return selectedPaletteIndex;
    }

    const std::vector<std::string>& GetPaletteOrder() const
    {
        return paletteOrder;
    }

    void PrintPalette() const
    {
        std::cout << "\n========== OBJECT PALETTE ==========" << std::endl;
        int index = 0;
        for (const auto& name : paletteOrder)
        {
            const ObjectDef* def = Get(name);
            if (def)
            {
                std::cout << "[" << index << "] " << def->displayName 
                          << " (" << def->category << ") - ID: " << def->id << std::endl;
            }
            index++;
        }
        std::cout << "==================================\n" << std::endl;
        std::cout << "Press number key to select object for spawning\n" << std::endl;
    }

    void PrintStats() const
    {
        std::cout << "\n--- Palette Stats ---" << std::endl;
        std::cout << "Total spawnable objects: " << paletteOrder.size() << std::endl;

        std::map<std::string, int> categoryCounts;
        for (const auto& name : paletteOrder)
        {
            const ObjectDef* def = Get(name);
            if (def)
                categoryCounts[def->category]++;
        }

        for (const auto& pair : categoryCounts)
        {
            std::cout << "  " << pair.first << ": " << pair.second << std::endl;
        }
        std::cout << std::endl;
    }
};

#endif
