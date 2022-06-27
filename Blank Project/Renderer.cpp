#include "Renderer.h"
#include "../nclgl/Light.h"
#include "../nclgl/HeightMap.h"
#include "../nclgl/Shader.h"
#include "../nclgl/Camera.h"
#include "../nclgl/tree.h"
#include <random>
#include "../nclgl/MeshAnimation.h"
#include "../nclgl/MeshMaterial.h"

#define  SHADOWSIZE  2048
const  int  LIGHT_NUM = 32;
const  int  TREE_NUM = 32;



Renderer::Renderer(Window& parent) : OGLRenderer(parent) {
	std::mt19937 X(rand());
	std::mt19937 Y(rand());
	std::uniform_real_distribution<double> distribution(0.2, 0.8);

    sphere = Mesh::LoadFromMeshFile("Sphere.msh");

    quad = Mesh::GenerateQuad();

    heightMap = new  HeightMap(TEXTUREDIR"p.jpg");

    waterTex = SOIL_load_OGL_texture(
        TEXTUREDIR"water.tga", SOIL_LOAD_AUTO,
        SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);

    earthTex = SOIL_load_OGL_texture(
        TEXTUREDIR"forrest_ground_01_diff_1k.jpg", SOIL_LOAD_AUTO,
        SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);

    earthBump = SOIL_load_OGL_texture(
        TEXTUREDIR"forrest_ground_01_nor_gl_1k.jpg", SOIL_LOAD_AUTO,
        SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);

    sandTex = SOIL_load_OGL_texture(
        TEXTUREDIR"aerial_beach_01_diff_1k.jpg", SOIL_LOAD_AUTO,
        SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);

    sandBump = SOIL_load_OGL_texture(
        TEXTUREDIR"aerial_beach_01_nor_gl_1k.jpg", SOIL_LOAD_AUTO,
        SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);

    snowTex = SOIL_load_OGL_texture(
        TEXTUREDIR"snow.jpg", SOIL_LOAD_AUTO,
        SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);

    snowBump = SOIL_load_OGL_texture(
        TEXTUREDIR"snow_nor_2.png", SOIL_LOAD_AUTO,
        SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);

    

    cubeMap = SOIL_load_OGL_cubemap(
        TEXTUREDIR"rusted_west.jpg", TEXTUREDIR"rusted_east.jpg",
        TEXTUREDIR"rusted_up.jpg", TEXTUREDIR"rusted_down.jpg",
        TEXTUREDIR"rusted_south.jpg", TEXTUREDIR"rusted_north.jpg",
        SOIL_LOAD_RGB, SOIL_CREATE_NEW_ID, 0);

    if (!earthTex || !earthBump || !cubeMap || !waterTex) {
        return;
    }


    SetTextureRepeating(earthTex, true);
    SetTextureRepeating(earthBump, true);

    SetTextureRepeating(snowTex, true);
    SetTextureRepeating(snowBump, true);

    SetTextureRepeating(sandTex, true);
    SetTextureRepeating(sandBump, true);

    SetTextureRepeating(waterTex, true);


    reflectShader = new  Shader(
        "reflectVertex.glsl", "reflectFragment.glsl");
    skyboxShader = new  Shader(
        "skyboxVertex.glsl", "skyboxFragment.glsl");

    lightShader = new  Shader(
        "BumpVertex.glsl", "BumpFragment.glsl");


    SceneShadowShader = new  Shader("shadowscenevert.glsl",
        "shadowscenefrag.glsl");
    shadowShader = new  Shader("shadowVert.glsl", "shadowFrag.glsl");

    if (!SceneShadowShader->LoadSuccess() || !shadowShader->LoadSuccess()) {
        return;
    }

    glGenTextures(1, &shadowTex);
    glBindTexture(GL_TEXTURE_2D, shadowTex);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
        SHADOWSIZE, SHADOWSIZE, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

    glBindTexture(GL_TEXTURE_2D, 0);

    glGenFramebuffers(1, &shadowFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
        GL_TEXTURE_2D, shadowTex, 0);
    glDrawBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    sceneMeshes.emplace_back(Mesh::GenerateQuad());

    sceneMeshes.emplace_back(Mesh::LoadFromMeshFile("T1.msh"));
    

    sceneDiffuse = SOIL_load_OGL_texture(TEXTUREDIR"BARK1.JPG",
        SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);

    sceneBump = SOIL_load_OGL_texture(TEXTUREDIR"BARK1_NOR.jpg",
        SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);


    SetTextureRepeating(sceneDiffuse, true);
    SetTextureRepeating(sceneBump, true);

    sceneShader = new  Shader("BumpVertex.glsl", // reused!
        "bufferFragment.glsl");
    pointlightShader = new  Shader("pointlightvert.glsl",
        "pointlightfrag.glsl");
    combineShader = new  Shader("combinevert.glsl",
        "combinefrag.glsl"); 

   

   

   if (!sceneShader->LoadSuccess() || !pointlightShader->LoadSuccess() ||
        !combineShader->LoadSuccess()) {
        return;
    }


    if (!reflectShader->LoadSuccess() ||
        !skyboxShader->LoadSuccess() ||
        !lightShader->LoadSuccess()) {
        return;
    }

    Vector3  heightmapSize = heightMap->GetHeightmapSize();
    
    camera = new  Camera(0.0f, 0.0f,
        heightmapSize * Vector3(0.5f, 1.0f, 1.1f));
   
    light = new  Light(heightmapSize * Vector3(0.5f, 1.5f, 0.8f),
        Vector4(1, 1, 1, 1), heightmapSize.x*1000);

    projMatrix = Matrix4::Perspective(1.0f, 15000.0f,
        (float)width / (float)height, 45.0f);

   sceneTransforms.resize(2);
  //  sceneTransforms[0] = Matrix4::Rotation(100, Vector3(0, 0, 1)) *
    //    Matrix4::Scale(Vector3(50, 50, 20));

  

    pointtree = new  tree[TREE_NUM];
    for (int j = 0; j < TREE_NUM; ++j) {
        tree& t = pointtree[j];
        t.SetPosition(Vector3(distribution(X) * heightmapSize.x,
			10.0f,
			distribution(Y)* heightmapSize.z));
    }

    sceneTime = 0.0f;

    pointLights = new  Light[LIGHT_NUM];

    for (int i = 0; i < LIGHT_NUM; i++) {
        Light& l = pointLights[i];
        l.SetPosition(Vector3(rand() % (int)heightmapSize.x,
            150.0f,
            rand() % (int)heightmapSize.z));

		l.SetColour(Vector4(1, 1, 1, 1));

       /* l.SetColour(Vector4(0.5f + (float)(rand() / (float)RAND_MAX),
            0.5f + (float)(rand() / (float)RAND_MAX),
            0.5f + (float)(rand() / (float)RAND_MAX),
            1));*/
        l.SetRadius(550.0f + (rand() % 250));
        
    }
    glGenFramebuffers(1, &bufferFBO);
    glGenFramebuffers(1, &pointLightFBO);

    GLenum  buffers[2] = {
        GL_COLOR_ATTACHMENT0 ,
        GL_COLOR_ATTACHMENT1
    };
    // Generate  our  scene  depth  texture ...
    GenerateScreenTexture(bufferDepthTex, true);
    GenerateScreenTexture(bufferColourTex);
    GenerateScreenTexture(bufferNormalTex);
    GenerateScreenTexture(lightDiffuseTex);
    GenerateScreenTexture(lightSpecularTex);

    //And  now  attach  them to our  FBOs
    glBindFramebuffer(GL_FRAMEBUFFER, bufferFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_2D, bufferColourTex, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1,
        GL_TEXTURE_2D, bufferNormalTex, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
        GL_TEXTURE_2D, bufferDepthTex, 0);
    glDrawBuffers(2, buffers);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) !=
        GL_FRAMEBUFFER_COMPLETE) {
        return;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, pointLightFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_2D, lightDiffuseTex, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1,
        GL_TEXTURE_2D, lightSpecularTex, 0);
    glDrawBuffers(2, buffers);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) !=
        GL_FRAMEBUFFER_COMPLETE) {
        return;
    }








    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    waterRotate = 0.0f;
    waterCycle = 0.0f;

    shader = new  Shader("SkinningVertex.glsl", "texturedFragment.glsl");

    if (!shader->LoadSuccess()) {
        return;
    }


    mesh = Mesh::LoadFromMeshFile("Role_T.msh");
    anim = new  MeshAnimation("Role_T.anm");
    material = new  MeshMaterial("Role_T.mat");

    for (int i = 0; i < mesh->GetSubMeshCount(); ++i) {
        const  MeshMaterialEntry* matEntry =
            material->GetMaterialForLayer(i);

        const  string* filename = nullptr;
        matEntry->GetEntry("Diffuse", &filename);
        string  path = TEXTUREDIR + *filename;
        GLuint  texID = SOIL_load_OGL_texture(path.c_str(), SOIL_LOAD_AUTO,
            SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y);
        matTextures.emplace_back(texID);
    }
    currentFrame = 0;
    frameTime = 0.0f;
    init = true;
}


void  Renderer::GenerateScreenTexture(GLuint& into, bool  depth) {
	glGenTextures(1, &into);
	glBindTexture(GL_TEXTURE_2D, into);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	GLuint  format = depth ? GL_DEPTH_COMPONENT24 : GL_RGBA8;
	GLuint  type = depth ? GL_DEPTH_COMPONENT : GL_RGBA;
	glTexImage2D(GL_TEXTURE_2D, 0,
		format, width, height, 0, type, GL_UNSIGNED_BYTE, NULL);

	glBindTexture(GL_TEXTURE_2D, 0);
}

Renderer ::~Renderer(void) {
    delete  camera;
    delete  heightMap;
    delete  quad;
    delete  reflectShader;
    delete  skyboxShader;
    delete  lightShader;
    delete  light;
    delete        sphere;
    glDeleteTextures(1, &shadowTex);
    glDeleteFramebuffers(1, &shadowFBO);

    for (auto& i : sceneMeshes) {
        delete i;
    }
    delete        sceneShader;
    delete        combineShader;
    delete        pointlightShader;
    delete[]     pointLights;
    glDeleteTextures(1, &bufferColourTex);
    glDeleteTextures(1, &bufferNormalTex);
    glDeleteTextures(1, &bufferDepthTex);
    glDeleteTextures(1, &lightDiffuseTex);
    glDeleteTextures(1, &lightSpecularTex);
    glDeleteFramebuffers(1, &bufferFBO);
    glDeleteFramebuffers(1, &pointLightFBO);

    

    delete  mesh;
    delete  anim;
    delete  material;
    delete  shader;
}

void  Renderer::UpdateScene(float dt) {
    
    camera->UpdateCamera(dt);
    viewMatrix = camera->BuildViewMatrix();
    waterRotate += dt * 2.0f; //2 degrees a second
    waterCycle += dt * 0.25f; //10  units a second

    frameTime -= dt;
    while (frameTime < 0.0f) {
        currentFrame = (currentFrame + 1) % anim->GetFrameCount();
        frameTime += 1.0f / anim->GetFrameRate();
    }

    for (int j = 0; j < TREE_NUM; ++j) {
        tree& t = pointtree[j];
        sceneTransforms[1] = Matrix4::Translation(t.GetPosition()) *
            Matrix4::Rotation(0, Vector3(1, 0, 0));

       
    }
    
}

void  Renderer::RenderScene() {
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    
   


    DrawSkybox();
    //FillBuffers();
   
    DrawShadowScene();
    DrawMainScene();
    
   
    DrawHeightmap();
   // DrawPointLights();
   // CombineBuffers();
    DrawWater();
    AnimSC();
    
}

void   Renderer::DrawSkybox() {
    glDepthMask(GL_FALSE);

    BindShader(skyboxShader);

    UpdateShaderMatrices();

    quad->Draw();

    glDepthMask(GL_TRUE);
}

void   Renderer::DrawHeightmap() {

   


    BindShader(lightShader);
    SetShaderLight(*light);
    glUniform3fv(glGetUniformLocation(lightShader->GetProgram(),
        "cameraPos"), 1, (float*)&camera->GetPosition());

    glUniform1i(glGetUniformLocation(
        lightShader->GetProgram(), "diffuseTex"), 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, earthTex);

    glUniform1i(glGetUniformLocation(
        lightShader->GetProgram(), "bumpTex"), 1);
    glActiveTexture(GL_TEXTURE0+1);
    glBindTexture(GL_TEXTURE_2D, earthBump);

	 glUniform1i(glGetUniformLocation(
        lightShader->GetProgram(), "diffuseTex1"), 2);
    glActiveTexture(GL_TEXTURE0 + 2);
    glBindTexture(GL_TEXTURE_2D, sandTex);

    glUniform1i(glGetUniformLocation(
        lightShader->GetProgram(), "bumpTex1"), 3);
    glActiveTexture(GL_TEXTURE0 + 3);
    glBindTexture(GL_TEXTURE_2D, sandBump);


	 glUniform1i(glGetUniformLocation(
        lightShader->GetProgram(), "snowTex"), 4);
    glActiveTexture(GL_TEXTURE0+4);
    glBindTexture(GL_TEXTURE_2D, snowTex);

    glUniform1i(glGetUniformLocation(
        lightShader->GetProgram(), "snowBump"), 5);
    glActiveTexture(GL_TEXTURE0+5);
    glBindTexture(GL_TEXTURE_2D, snowBump);  //multiple textures 


    modelMatrix.ToIdentity();      //New!
    textureMatrix.ToIdentity();    //New!


    UpdateShaderMatrices();

    heightMap->Draw();
   
}

void   Renderer::DrawWater() {

    BindShader(reflectShader);

    glUniform3fv(glGetUniformLocation(reflectShader->GetProgram(),
        "cameraPos"), 1, (float*)&camera->GetPosition());

    glUniform1i(glGetUniformLocation(
        reflectShader->GetProgram(), "diffuseTex"), 0);
    glUniform1i(glGetUniformLocation(
        reflectShader->GetProgram(), "cubeTex"), 2);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, waterTex);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap);
    Vector3  hSize = heightMap->GetHeightmapSize();

    modelMatrix =
        Matrix4::Translation(Vector3(0.97, 0.006, 0.97) * 2100.2f) *
        Matrix4::Scale(hSize * 0.8f) *
        Matrix4::Rotation(90, Vector3(1, 0, 0));

    textureMatrix =
        Matrix4::Translation(Vector3(waterCycle, 0.0f, waterCycle)) *
        Matrix4::Scale(Vector3(10, 10, 10)) *
        Matrix4::Rotation(waterRotate, Vector3(0, 0, 1));

    UpdateShaderMatrices();
    // SetShaderLight (* light); //No  lighting  in this  shader!
    quad->Draw();
}

void   Renderer::FillBuffers() {
    glBindFramebuffer(GL_FRAMEBUFFER, bufferFBO);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    BindShader(sceneShader);
   // SetShaderLight(*light);
    glUniform1i(
        glGetUniformLocation(sceneShader->GetProgram(), "diffuseTex"), 0);
    glUniform1i(
        glGetUniformLocation(sceneShader->GetProgram(), "bumpTex"), 1);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, earthTex);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, earthBump);

    modelMatrix.ToIdentity();
    viewMatrix = camera->BuildViewMatrix();
    projMatrix = Matrix4::Perspective(1.0f, 10000.0f,
        (float)width / (float)height, 45.0f);

    UpdateShaderMatrices();

    heightMap->Draw();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void  Renderer::DrawPointLights() {

    glBindFramebuffer(GL_FRAMEBUFFER, pointLightFBO);
    BindShader(pointlightShader);

    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glBlendFunc(GL_ONE, GL_ONE);
    glCullFace(GL_FRONT);
    glDepthFunc(GL_ALWAYS);
    glDepthMask(GL_FALSE);

    glUniform1i(glGetUniformLocation(
        pointlightShader->GetProgram(), "depthTex"), 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, bufferDepthTex);

    glUniform1i(glGetUniformLocation(
        pointlightShader->GetProgram(), "normTex"), 1);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, bufferNormalTex);

    glUniform3fv(glGetUniformLocation(pointlightShader->GetProgram(),
        "cameraPos"), 1, (float*)&camera->GetPosition());

    glUniform2f(glGetUniformLocation(pointlightShader->GetProgram(),
        "pixelSize"), 1.0f / width, 1.0f / height);

    Matrix4  invViewProj = (projMatrix * viewMatrix).Inverse();
    glUniformMatrix4fv(glGetUniformLocation(
        pointlightShader->GetProgram(), "inverseProjView"),
        1, false, invViewProj.values);

    UpdateShaderMatrices();
    for (int i = 0; i < LIGHT_NUM; ++i) {
        Light& l = pointLights[i];
        SetShaderLight(l);
        sphere->Draw();
    }

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glCullFace(GL_BACK);
    glDepthFunc(GL_LEQUAL);

    glDepthMask(GL_TRUE);

    glClearColor(0.2f, 0.2f, 0.2f, 1);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

}

void  Renderer::CombineBuffers() {
    BindShader(combineShader);
    modelMatrix.ToIdentity();
    viewMatrix.ToIdentity();
    projMatrix.ToIdentity();
    UpdateShaderMatrices();

    glUniform1i(glGetUniformLocation(
        combineShader->GetProgram(), "diffuseTex"), 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, bufferColourTex);

    glUniform1i(glGetUniformLocation(
        combineShader->GetProgram(), "diffuseLight"), 1);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, lightDiffuseTex);

    glUniform1i(glGetUniformLocation(
        combineShader->GetProgram(), "specularLight"), 2);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, lightSpecularTex);

    quad->Draw();
}

void  Renderer::DrawShadowScene() {

    glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);

    glClear(GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, SHADOWSIZE, SHADOWSIZE);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

    BindShader(shadowShader);

    viewMatrix = Matrix4::BuildViewMatrix(
        light->GetPosition(), Vector3(0, 0, 0));
    projMatrix = Matrix4::Perspective(1, 100, 1, 45);
    shadowMatrix = projMatrix * viewMatrix; //used  later

   /* for (int i = 0; i < 2; ++i) {
        modelMatrix = sceneTransforms[i];
        UpdateShaderMatrices();
        sceneMeshes[i]->Draw();
    }*/


    for (int j = 0; j < TREE_NUM; ++j) {
        tree& t = pointtree[j];
        modelMatrix = Matrix4::Translation(t.GetPosition()) *
            Matrix4::Scale(Vector3(25, 10, 20));
        UpdateShaderMatrices();
        sceneMeshes[0]->Draw();
    }

    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glViewport(0, 0, width, height);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}  //draw real time shadows

void  Renderer::DrawMainScene() {

    BindShader(SceneShadowShader);
    SetShaderLight(*light);
    viewMatrix = camera->BuildViewMatrix();
    projMatrix = Matrix4::Perspective(1.0f, 15000.0f,
        (float)width / (float)height, 45.0f);

    glUniform1i(glGetUniformLocation(SceneShadowShader->GetProgram(),
        "diffuseTex"), 0);
    glUniform1i(glGetUniformLocation(SceneShadowShader->GetProgram(),
        "bumpTex"), 1);
    glUniform1i(glGetUniformLocation(SceneShadowShader->GetProgram(),
        "shadowTex"), 2);

    glUniform3fv(glGetUniformLocation(SceneShadowShader->GetProgram(),
        "cameraPos"), 1, (float*)&camera->GetPosition());

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sceneDiffuse);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, sceneBump);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, shadowTex);

   /* for (int i = 0; i < 2; ++i) {
        modelMatrix = sceneTransforms[0];
        UpdateShaderMatrices();
        sceneMeshes[i]->Draw();
    }*/

    for (int j = 0; j < TREE_NUM; ++j) {
        tree& t = pointtree[j];
        modelMatrix = Matrix4::Translation(t.GetPosition()) *
			Matrix4::Scale(Vector3(25, 15, 20));
		
        UpdateShaderMatrices();
        sceneMeshes[1]->Draw();
    }
	
}

void  Renderer::AnimSC() {
    //glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    BindShader(shader);
    glUniform1i(glGetUniformLocation(shader->GetProgram(),
        "diffuseTex"), 0);

    UpdateShaderMatrices();

    vector <Matrix4 > frameMatrices;

    const  Matrix4* invBindPose = mesh->GetInverseBindPose();
    const  Matrix4* frameData = anim->GetJointData(currentFrame);

    for (unsigned int i = 0; i < mesh->GetJointCount(); ++i) {
        frameMatrices.emplace_back(frameData[i] * invBindPose[i]);
    }

    int j = glGetUniformLocation(shader->GetProgram(), "joints");
    glUniformMatrix4fv(j, frameMatrices.size(), false,
        (float*)frameMatrices.data());

   
    for (int i = 0; i < mesh->GetSubMeshCount(); ++i) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, matTextures[i]);

        Vector3  hSize = heightMap->GetHeightmapSize();

        modelMatrix =
            Matrix4::Translation(Vector3(1.0, 0.028, 1.0) * 2100.2f) *
            Matrix4::Scale(Vector3(40, 100,30)) *
            Matrix4::Rotation(0, Vector3(0, 1,0));

        textureMatrix =
            Matrix4::Translation(Vector3(waterCycle, 0.0f, waterCycle)) *
            Matrix4::Scale(Vector3(10, 10, 10)) *
            Matrix4::Rotation(waterRotate, Vector3(0, 0, 1));
        UpdateShaderMatrices();
        mesh->DrawSubMesh(i);

       
    }
   


}

