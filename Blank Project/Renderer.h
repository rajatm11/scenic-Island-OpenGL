#pragma once
#include "../nclgl/OGLRenderer.h"
class  MeshAnimation;
class  MeshMaterial;
class  HeightMap;
class  Camera;
class Light;
class tree;
class  Mesh;
class Renderer : public OGLRenderer	{
public:
	Renderer(Window &parent);
	 ~Renderer(void);
	 void RenderScene()				override;
	 void UpdateScene(float dt)	override;
	 
protected:
	void          DrawHeightmap();
	void          DrawWater();
	void          DrawSkybox();
	void AnimSC();
	void   FillBuffers();            //G-Buffer  Fill  Render  Pass
	void   DrawPointLights();       // Lighting  Render  Pass
	void   CombineBuffers();         // Combination  Render  Pass
	
	void   GenerateScreenTexture(GLuint& into, bool  depth = false);
   

	Shader* sceneShader;           // Shader  to fill  our  GBuffers
	Shader* pointlightShader;     // Shader  to  calculate  lighting
	Shader* combineShader;         // shader  to  stick it all  together


	Shader* lightShader;
	Shader* reflectShader;
	Shader* skyboxShader;

	GLuint    bufferFBO;             //FBO  for  our G-Buffer  pass
	GLuint    bufferColourTex;      // Albedo  goes  here
	GLuint    bufferNormalTex;      // Normals  go here
	GLuint    bufferDepthTex;       // Depth  goes  here
	GLuint    pointLightFBO;         //FBO  for  our  lighting  pass
	GLuint    lightDiffuseTex;      // Store  diffuse  lighting
	GLuint    lightSpecularTex;     // Store  specular  lighting


	HeightMap * heightMap;
	
	Mesh* quad;

	
	
	Camera* camera;
	Light* light;
	GLuint        cubeMap;
	Light* pointLights;           // Array of  lighting  data
	Mesh* sphere;                 // Light  volume
	GLuint        waterTex;

	GLuint        earthTex;
	GLuint        earthBump;

	GLuint        snowTex;
	GLuint        snowBump;

	GLuint        sandTex;
	GLuint        sandBump;

	Shader* SceneShadowShader;
	Shader* shadowShader;

	float     waterRotate;
	float     waterCycle;

	vector <Mesh*>      sceneMeshes;
	vector <Matrix4 >    sceneTransforms;
	GLuint    shadowTex;
	GLuint    shadowFBO;

	void   DrawShadowScene();
	void   DrawMainScene();
	GLuint    sceneDiffuse;
	GLuint    sceneBump;
	float     sceneTime;
	tree* pointtree;
	Camera* Cam;

	Mesh* mesh;
	Shader* shader;
	MeshAnimation* anim;
	MeshMaterial* material;
	vector <GLuint > matTextures;



	
	int    currentFrame;
	float  frameTime;
};
