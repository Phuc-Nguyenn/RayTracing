#pragma once

class TextureUnitManager {
    private:
        static unsigned int textureUnit;
    public:
        TextureUnitManager() = delete;
        TextureUnitManager(const TextureUnitManager& other) = delete;
        TextureUnitManager& operator=(const TextureUnitManager& other) = delete;

        static unsigned int getNewTextureUnit() {
            return ++textureUnit;
        }

        static unsigned int getCurrentTextureUnit() {
            return textureUnit;
        }
};