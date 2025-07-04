#pragma once

#include <cstdlib>
#include <iostream>

class TextureUnitManager {
    private:
        static unsigned int textureUnit;
        static unsigned int maxTextureUnits;
    public:
        TextureUnitManager() = delete;
        TextureUnitManager(const TextureUnitManager& other) = delete;
        TextureUnitManager& operator=(const TextureUnitManager& other) = delete;

        static unsigned int getNewTextureUnit() {
            if(++textureUnit >= maxTextureUnits) {
                std::cout << "next texture unit exceed maximum texture units limit " << std::endl;
                std::abort();
            }
            return textureUnit;
        }

        static unsigned int getCurrentTextureUnit() {
            return textureUnit;
        }

        static void ResetTextureUnits() {
            textureUnit = 0;
        }
};