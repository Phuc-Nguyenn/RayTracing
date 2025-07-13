#pragma once

#include <vector>
#include <fstream>
#include <optional>

#include "Tri.h"
#include "Materials.h"
#include "Math3D.h"

class ObjectLoader
{
protected:
    std::string targetFilePath;
    std::unique_ptr<Material::Material> material;
    std::vector<Tri> triangles;
    static int materialsLoaded;
    int myMaterialIndex;
    float scale;
    Vector3f position;
    std::fstream vtxStream;
    std::string format;

    bool handlePositionArg() {
        if(!(vtxStream >> position.x >> position.z >> position.y)) {
            return false;
        }
        return true;
    }

    bool handleScaleArg() {
        if(!(vtxStream >> scale)) {
            return false;
        }
        return true;
    }

    bool handleFormatArg() {
        if(!(vtxStream >> format)) {
            return false;
        }
        if(format != "xyz" && format != "xzy") {
            std::cout << "format must be xyz | xzy" << std::endl;
            format = "xyz";
            return false;
        }
        return true;
    }

public:
    ObjectLoader() : position(Vector3f(0,0,0)), scale(1), format("xyz") {
    }

    bool TargetFile(const std::string& filePath) {
        targetFilePath = filePath;
        position = Vector3f(0,0,0);
        scale = 1;
        myMaterialIndex = -1;
        vtxStream.close();
        vtxStream.open(targetFilePath);
        if (!vtxStream.is_open()) {
            std::cerr << "Failed to open file: " << targetFilePath << std::endl;
            return false;
        }
        return true;
    }

    virtual std::optional<Material::Material> ExtractMaterial() {
        // Read in position and material from object file
        std::string materialType;
        std::string arg;
        while (vtxStream >> arg) {
            if (arg == "position") {
                if (!handlePositionArg()) {
                    std::cout << "failed to read <position> argument of file: " << targetFilePath << std::endl;
                }
            } else if (arg == "scale") {
                if (!handleScaleArg()) {
                    std::cout << "failed to read <scale> argument of file: " << targetFilePath << std::endl;
                }
            } else if (arg == "format") {
                if(!handleFormatArg()) {
                    std::cout << "failed to read <format> argument of file: " << targetFilePath << std::endl;
                }
            } else {
                materialType = arg;
                break;
            }
        }

        bool status = true;

        if(materialType == "default-material") {
            material = std::make_unique<Material::Lambertian>(Vector3f{0.75, 0.75, 0.75});
        } else {
            Vector3f color;
            if (!(vtxStream >> color.x >> color.y >> color.z)) {
                std::cout << "unable to read material color in " << targetFilePath << std::endl;
                return {};
            }
            if(materialType == "specular") {
                float roughness;
                status = bool(vtxStream >> roughness);
                if(status) material = std::make_unique<Material::Specular>(color, roughness); 
            } else if(materialType == "lambertian") {
                if(status) material = std::make_unique<Material::Lambertian>(color);
            } else if(materialType == "metallic") {
                float roughness, metallic;
                status = bool(vtxStream >> roughness >> metallic);
                if(status) material = std::make_unique<Material::Metallic>(color, roughness, metallic);
            } else if(materialType == "transparent") {
                float transparency, refractionIndex;
                status = bool(vtxStream >> transparency >> refractionIndex);
                if(status) material = std::make_unique<Material::Transparent>(color, transparency, refractionIndex);
            } else if(materialType == "lightsource") {
                float luminescent = 1.0;
                status = bool(vtxStream >> luminescent);
                if(status) material = std::make_unique<Material::LightSource>(color, luminescent);
            } else {
                std::cout << "unknown material type: " << materialType << std::endl;
                return {};
            }
        }
        if(!status) {
            std::cout << "unable to read material format" << std::endl;
            return {};
        }
        myMaterialIndex = materialsLoaded++;
        return {*material};
    };

    virtual std::optional<std::vector<Tri>> ExtractTriangles() {
        Vector3f x1;
        Vector3f x2;
        Vector3f x3;
        int n;
        vtxStream >> n;
        for(int i=0; i<n; ++i) {
            if(vtxStream >> x1.x >> x1.y >> x1.z >> x2.x >> x2.y >> x2.z >> x3.x >> x3.y >> x3.z) {
                if(format == "xzy") {
                    std::swap(x1.y, x1.z);
                    std::swap(x2.y, x2.z);
                    std::swap(x3.y, x3.z);
                }
                x1 = x1*scale + position; 
                x2 = x2*scale + position; 
                x3 = x3*scale + position;
                triangles.push_back(Tri(x1, x2, x3, myMaterialIndex));
            } else {
                std::cout << "failed to read " << targetFilePath << " on vertex " << i + 1 << std::endl;
                return {};
            }
        }
        return {triangles};
    };
};

class OFFLoader : public ObjectLoader
{
private:
    std::vector<Vector3f> vertices;
public:
    virtual std::optional<std::vector<Tri>> ExtractTriangles() override {
        Vector3f x1;
        Vector3f x2;
        Vector3f x3;
        std::string offheader;
        vtxStream >> offheader;
        if(offheader != "OFF") {
            std::cout << "failed: expected .off to have OFF header\nformat is <material information> OFF <vertices count> <faces count> <edges count> ... " << std::endl;
        }
        int verticesCount, facesCount, edgesCount;
        if(!(vtxStream >> verticesCount >> facesCount >> edgesCount)) {
            std::cout << "failed to get <vertices count> <faces count> <edges count> in .off file: " << targetFilePath << std::endl;
            return {};
        }
        for(int i=0; i<verticesCount; ++i) {
            Vector3f vertex;
            if(!(vtxStream >> vertex.x >> vertex.y >> vertex.z)) {
                std::cout << "failed to read vertex on line " << i + 1 << " of .off file: " << targetFilePath << std::endl;
                return {};
            }
            if(format == "xzy") {
                std::swap(vertex.y, vertex.z);
            }
            vertices.push_back(vertex*scale + position);
        }
        if(facesCount != 0) {
            for(int i=0; i<facesCount; ++i) {
                int n,a,b,c;
                if(!(vtxStream >> n >> a >> b >> c)) {
                    std::cout << "failed to read face on line " << i + 1 << " of .off file: " << targetFilePath << std::endl;
                    return {};
                }
                triangles.push_back(Tri(vertices[a], vertices[b], vertices[c], myMaterialIndex));
            }
        } else {
            // For each vertex, create a small triangular prism (as 2 triangles forming a tiny tetrahedron)
            const float cubeLen = 0.02f;
            for(int i=0; i<verticesCount; ++i) {
                Vector3f center = vertices[i];
                // 8 corners of the cube
                Vector3f corners[8];
                float h = cubeLen * 0.5f;
                corners[0] = center + Vector3f(-h, -h, -h);
                corners[1] = center + Vector3f( h, -h, -h);
                corners[2] = center + Vector3f( h,  h, -h);
                corners[3] = center + Vector3f(-h,  h, -h);
                corners[4] = center + Vector3f(-h, -h,  h);
                corners[5] = center + Vector3f( h, -h,  h);
                corners[6] = center + Vector3f( h,  h,  h);
                corners[7] = center + Vector3f(-h,  h,  h);
                triangles.push_back(Tri(corners[0], corners[1], corners[2], myMaterialIndex));
                triangles.push_back(Tri(corners[0], corners[2], corners[3], myMaterialIndex));
                triangles.push_back(Tri(corners[4], corners[5], corners[6], myMaterialIndex));
                triangles.push_back(Tri(corners[4], corners[6], corners[7], myMaterialIndex));
                triangles.push_back(Tri(corners[0], corners[1], corners[5], myMaterialIndex));
                triangles.push_back(Tri(corners[0], corners[5], corners[4], myMaterialIndex));
                triangles.push_back(Tri(corners[3], corners[2], corners[6], myMaterialIndex));
                triangles.push_back(Tri(corners[3], corners[6], corners[7], myMaterialIndex));
                triangles.push_back(Tri(corners[0], corners[3], corners[7], myMaterialIndex));
                triangles.push_back(Tri(corners[0], corners[7], corners[4], myMaterialIndex));
                triangles.push_back(Tri(corners[1], corners[2], corners[6], myMaterialIndex));
                triangles.push_back(Tri(corners[1], corners[6], corners[5], myMaterialIndex));
            }
        }
        return {triangles};
    };
};