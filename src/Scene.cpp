#include "Scene.h"

Scene::Scene(std::vector<unsigned int> shaderProgramIds) : 
    shaderProgramIds(shaderProgramIds), 
    shaderProgramId(shaderProgramIds[TRACER_ID]),
    frameIndex(0), 
    objectsIndex(0), 
    inFpsTest(false), 
    fpsTestAngle(0),
    inBoxHitView(false), 
    camera(*this),
    bounceLimitManager(*this, shaderProgramId)
{
    if(shaderProgramIds.size() < 2) {
        std::cerr << "Error: At least two shader program IDs are required." << std::endl;
        throw std::runtime_error("Insufficient shader program IDs");
    }
    // set the default values
    GLCALL(glUniform1ui(glGetUniformLocation(shaderProgramId, "u_BounceLimit"), 4));
    GLCALL(glUniform1f(GetUniformLocation("u_Camera.viewportWidth"), camera.GetViewportWidth()));
    GLCALL(glUniform1f(GetUniformLocation("u_Camera.viewportDistance"), camera.GetViewportDistance()));
    GLCALL(glUniform1f(GetUniformLocation("u_Camera.fov"), camera.GetFov()));
}

void Scene::Tick() {    
    // tick the camera and upload
    camera.Tick();

    // other scene stuff
    GLCALL(glUniform1ui(glGetUniformLocation(shaderProgramId, "u_FrameIndex"), frameIndex++));
    GLCALL(glUniform3f(glGetUniformLocation(shaderProgramId, "u_RandSeed"), float(std::rand()) / RAND_MAX, float(std::rand()) / RAND_MAX, float(std::rand()) / RAND_MAX));
}

std::vector<int> Scene::FlattenTrianglesMatIdx(const std::vector<Tri>& reorderedTris) {
    std::vector<int> flattened;
    flattened.reserve(reorderedTris.size());
    for(const auto& tri : reorderedTris) {
        flattened.push_back(tri.materialsIndex);
    }
    return flattened;
}

std::vector<float> Scene::FlattenTrianglesVertices(const std::vector<Tri>& reorderedTris) {
    std::vector<float> flattened;
    flattened.reserve(reorderedTris.size() * 12);
    for (const auto& tri : reorderedTris) {
        // pos1
        flattened.push_back(tri.pos1.x);
        flattened.push_back(tri.pos1.y);
        flattened.push_back(tri.pos1.z);
        flattened.push_back(0); // padding
        // pos2
        flattened.push_back(tri.pos2.x);
        flattened.push_back(tri.pos2.y);
        flattened.push_back(tri.pos2.z);
        flattened.push_back(0); // padding
        // pos3
        flattened.push_back(tri.pos3.x);
        flattened.push_back(tri.pos3.y);
        flattened.push_back(tri.pos3.z);
        flattened.push_back(0); // padding
    }
    return flattened;
}

std::vector<float> Scene::FlattenBoundingBoxes(const std::vector<BoundingBox>& boundingBoxes) {
    std::vector<float> flattened;
    flattened.reserve(boundingBoxes.size() * 9);
    for (const auto& box : boundingBoxes) {
        // Use 'mini' and 'maxi' as per the BoundingBox definition
        flattened.push_back(box.maxi.x);
        flattened.push_back(box.maxi.y);
        flattened.push_back(box.maxi.z);
        flattened.push_back(box.mini.x);
        flattened.push_back(box.mini.y);
        flattened.push_back(box.mini.z);
        flattened.push_back(static_cast<float>(box.triangleCount));
        if(box.triangleCount == 0) { //if leaf node then
            flattened.push_back(static_cast<float>(box.rightChildIndex));
        } else {
            flattened.push_back(static_cast<float>(box.triangleStartIndex));
        }
    }
    return flattened;
}

void Scene::LoadObjects(const std::vector<std::string>& objectFilePaths) {
    int numberOfFiles = objectFilePaths.size();
    std::unique_ptr<ObjectLoader> objectLoader;
    for(int i=0; i<numberOfFiles; ++i) {
        if(objectFilePaths[i].find(".off") != std::string::npos || objectFilePaths[i].find(".OFF") != std::string::npos) {
            objectLoader = std::make_unique<OFFLoader>();
        } else {
            objectLoader = std::make_unique<ObjectLoader>();
        }

        if(!objectLoader->TargetFile(objectFilePaths[i])) {
            std::cout << "unable to read from: " << objectFilePaths[i] << std::endl;
            continue;
        }
        std::optional<Material::Material> material = objectLoader->ExtractMaterial();
        if(!material.has_value()) {
            std::cout << "unable to read material from: " << objectFilePaths[i] << std::endl;
            continue;
        }
        materials.push_back(std::make_unique<Material::Material>(material.value()));
        std::optional<std::vector<Tri>> objTris = objectLoader->ExtractTriangles();
        if(!objTris.has_value()) {
            std::cout << "unable to read triangles from: " << objectFilePaths[i] << std::endl;
            continue;
        }
        triangles.insert(triangles.end(), objTris.value().begin(), objTris.value().end());
    }

    // create the Bvh tree
    BvhTree bvhtree(triangles);
    auto [boundingBoxes, reorderedTriangles] = bvhtree.BuildTree();
    triangles = reorderedTriangles;
    // Flatten all triangles into a single vector
    std::vector<float> trianglesVertexData = FlattenTrianglesVertices(triangles);
    std::vector<int> trianglesMatIdxData = FlattenTrianglesMatIdx(triangles);
    std::vector<float> boundingBoxesData = FlattenBoundingBoxes(boundingBoxes);
    // SendDataAsTextureBuffer(trianglesVertexData, triangles.size(), "u_Triangles", TextureUnitManager::getNewTextureUnit(), GL_RGB32F);
    SendDataAsSSBO(trianglesVertexData, 0, GL_STATIC_DRAW);
    SendDataAsTextureBuffer(trianglesMatIdxData, triangles.size(), "u_MaterialsIndex", TextureUnitManager::getNewTextureUnit(), GL_R32I);
    SendDataAsTextureBuffer(boundingBoxesData, boundingBoxes.size(), "u_BoundingBoxes", TextureUnitManager::getNewTextureUnit(), GL_RGBA32F);
    SendSceneMaterials();
    
    std::cout << "triangles count: " << triangles.size() << std::endl;
    std::cout << "floats count: " << trianglesVertexData.size() << std::endl;
}

void Scene::SendSceneMaterials() {
    for(int i=0; i<materials.size(); ++i)
    {
        const auto& material = *materials[i];
        auto colourLoc = GetUniformLocationIdx("u_Materials", i, {{"colour"}});
        auto specularcolourLoc = GetUniformLocationIdx("u_Materials", i, {{"specularColour"}});
        auto roughnessLoc = GetUniformLocationIdx("u_Materials", i, {{"roughness"}});
        auto metallicLoc = GetUniformLocationIdx("u_Materials", i, {{"metallic"}});
        auto transparencyLoc = GetUniformLocationIdx("u_Materials", i, {{"transparency"}});
        auto refractionIdxLoc = GetUniformLocationIdx("u_Materials", i, {{"refractionIndex"}});
        auto isLightLoc = GetUniformLocationIdx("u_Materials", i, {{"isLight"}});
        glUniform3f(colourLoc, material.colour.x, material.colour.y, material.colour.z);
        glUniform3f(specularcolourLoc, material.specularColour.x, material.specularColour.y, material.specularColour.z);
        glUniform1f(roughnessLoc, material.roughness);
        glUniform1f(metallicLoc, material.metallic);
        glUniform1f(transparencyLoc, material.transparency);
        glUniform1f(refractionIdxLoc, material.refractionIndex);
        glUniform1i(isLightLoc, material.isLight ? 1 : 0);
    }
    glUniform1i(glGetUniformLocation(shaderProgramId, "u_MaterialsCount"), materials.size()); 
}

unsigned int Scene::GetUniformLocation(std::string uname) {
    std::string query = uname;
    unsigned int result;
    GLCALL(result = glGetUniformLocation(shaderProgramId, query.c_str()));
    return result;
}

unsigned int Scene::GetUniformLocationIdx(std::string uname, unsigned int index, std::vector<std::string> attributes) {
    std::string indexStr = std::to_string(index);
    std::string query = uname + "[" + indexStr +"]";
    for(auto attribute : attributes) {
        query += "." + attribute;
    }
    unsigned int result;
    GLCALL(result = glGetUniformLocation(shaderProgramId, query.c_str()));
    return result;
};

