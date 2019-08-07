#include "World.h"

#include "StaticCamera.h"
#include "FirstPersonCamera.h"

#include "Renderer.h"
#include "CubeModel.h"
#include "SphereModel.h"
#include "Animation.h"
#include "Billboard.h"
#include <GLFW/glfw3.h>
#include "EventManager.h"
#include "TextureLoader.h"

#include "ParticleDescriptor.h"
#include "ParticleEmitter.h"
#include "ParticleSystem.h"

#include "LightSource.h"

World* World::worldInstance;
const float World::WorldBlockSize = 100;

void World::LoadScene(const char * scene_path) {
	//mWorldBlock->LoadScene(scene_path);

	// Using case-insensitive strings and streams for easier parsing
	ci_ifstream input;
	input.open(scene_path, ios::in);

	// Invalid file
	if (input.fail())
	{
		fprintf(stderr, "Error loading file: %s\n", scene_path);
		getchar();
		exit(-1);
	}

	ci_string item;
	while (std::getline(input, item, '['))
	{
		ci_istringstream iss(item);

		ci_string result;
		if (std::getline(iss, result, ']'))
		{
			if (result == "cube")
			{
				// Box attributes
				CubeModel* cube = new CubeModel();
				cube->Load(iss);
				mModel.push_back(cube);
			}
			else if (result == "sphere")
			{
				SphereModel* sphere = new SphereModel();
				sphere->Load(iss);
				mModel.push_back(sphere);
			}
			else if (result == "animationkey")
			{
				AnimationKey* key = new AnimationKey();
				key->Load(iss);
				mAnimationKey.push_back(key);
			}
			else if (result == "animation")
			{
				Animation* anim = new Animation();
				anim->Load(iss);
				mAnimation.push_back(anim);
			}
			else if (result == "particledescriptor")
			{
				ParticleDescriptor* psd = new ParticleDescriptor();
				psd->Load(iss);
				AddParticleDescriptor(psd);
			}
			else if (result.empty() == false && result[0] == '#')
			{
				// this is a comment line
			}
			else if (result == "light") {
				LightSource* light = new LightSource();
				light->Load(iss);
				lightSource.push_back(light);
			}
			else
			{
				fprintf(stderr, "Error loading scene file... !");
				getchar();
				exit(-1);
			}
		}
	}
	input.close();

	// Set Animation vertex buffers
	for (vector<Animation*>::iterator it = mAnimation.begin(); it < mAnimation.end(); ++it)
	{
		// Draw model
		(*it)->CreateVertexBuffer();
	}

	//setupWorldBlock(mWorldBlock0);
	//setupWorldBlock(mWorldBlock1);
	//setupWorldBlock(mWorldBlock2);
	//setupWorldBlock(mWorldBlock3);
	//setupWorldBlock(mWorldBlock4);
	//setupWorldBlock(mWorldBlock5);
	//setupWorldBlock(mWorldBlock6);
	//setupWorldBlock(mWorldBlock7);
	//setupWorldBlock(mWorldBlock8);

	setupWorldBlock(mWorldBlock[0]);
	checkNeighbors();
}

void World::setupWorldBlock(WorldBlock* WB) {
	
	WB->setAnimationKey(mAnimationKey);
	WB->setAnimation(mAnimation);
	WB->setParticleSystemList(mParticleSystemList);
	WB->setParticleDescriptorList(mParticleDescriptorList);

	WB->setModel(mModel);
	
	//WB->setCamera(mCamera);
	//WB->setCurrentCamera(mCurrentCamera);
	WB->setLightSource(lightSource);
	WB->setBillboardList(mpBillboardList);

}

void World::Update(float dt) {

	// Update current Camera
	mCamera[mCurrentCamera]->Update(dt);

	// check for the center block
	int x = floor((mCharacterPosition.x + 50) / 100);
	int z = floor((mCharacterPosition.z + 50) / 100);
	vec2 newCenter = vec2(x, z);
	if (newCenter != CenterBlock) {
		CenterBlock = newCenter;
		checkNeighbors();
	}
	




	// User Inputs
	// 0 1 2 to change the Camera
	if (glfwGetKey(EventManager::GetWindow(), GLFW_KEY_1) == GLFW_PRESS)
	{
		mCurrentCamera = 0;
	}
	else if (glfwGetKey(EventManager::GetWindow(), GLFW_KEY_2) == GLFW_PRESS)
	{
		if (mCamera.size() > 1)
		{
			mCurrentCamera = 1;
		}
	}
	else if (glfwGetKey(EventManager::GetWindow(), GLFW_KEY_3) == GLFW_PRESS)
	{
		if (mCamera.size() > 2)
		{
			mCurrentCamera = 2;
		}
	}

	// Spacebar to change the shader
	if (glfwGetKey(EventManager::GetWindow(), GLFW_KEY_0) == GLFW_PRESS)
	{
		Renderer::SetShader(SHADER_SOLID_COLOR);
	}
	else if (glfwGetKey(EventManager::GetWindow(), GLFW_KEY_9) == GLFW_PRESS)
	{
		Renderer::SetShader(SHADER_BLUE);
	}

	for (vector<ParticleSystem*>::iterator it = mParticleSystemList.begin(); it != mParticleSystemList.end(); ++it)
	{
		(*it)->Update(dt);
	}

	mpBillboardList->Update(dt);

	mWorldBlock[0]->Update(dt);
	//mWorldBlock0->Update(dt);

}


void World::Draw() {

	glClearColor(0.1f, 0.13f, 0.2f,0.0f);
	//mWorldBlock0->Draw();
	//mWorldBlock1->Draw();

	//first shader
	Renderer::BeginFrame();
	// Set shader to use
	glUseProgram(Renderer::GetShaderProgramID());
	Renderer::CheckForErrors();

	for (int i = 0; i < 9; i++) {
		mWorldBlock[DisplayedWBIndex[i]]->DrawCurrentShader();
	}


	//mWorldBlock0->DrawCurrentShader();
	//mWorldBlock1->DrawCurrentShader();
	//mWorldBlock2->DrawCurrentShader();
	//mWorldBlock3->DrawCurrentShader();
	//mWorldBlock4->DrawCurrentShader();
	//mWorldBlock5->DrawCurrentShader();
	//mWorldBlock6->DrawCurrentShader();
	//mWorldBlock7->DrawCurrentShader();
	//mWorldBlock8->DrawCurrentShader();


	
	// path lines shader
	Renderer::CheckForErrors();
	unsigned int prevShader = Renderer::GetCurrentShader();
	Renderer::SetShader(SHADER_PATH_LINES);
	glUseProgram(Renderer::GetShaderProgramID());
	Renderer::CheckForErrors();

	for (int i = 0; i < 9; i++) {
		mWorldBlock[DisplayedWBIndex[i]]->DrawPathLinesShader();
	}

	//mWorldBlock0->DrawPathLinesShader();
	//mWorldBlock1->DrawPathLinesShader();
	//mWorldBlock2->DrawPathLinesShader();
	//mWorldBlock3->DrawPathLinesShader();
	//mWorldBlock4->DrawPathLinesShader();
	//mWorldBlock5->DrawPathLinesShader();
	//mWorldBlock6->DrawPathLinesShader();
	//mWorldBlock7->DrawPathLinesShader();
	//mWorldBlock8->DrawPathLinesShader();


	Renderer::CheckForErrors();
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	Renderer::CheckForErrors();

	ShaderType oldShader = (ShaderType)Renderer::GetCurrentShader();
	//Texture shader
	Renderer::SetShader(SHADER_TEXTURED);
	glUseProgram(Renderer::GetShaderProgramID());

	Renderer::CheckForErrors();

	for (int i = 0; i < 9; i++) {
		mWorldBlock[DisplayedWBIndex[i]]->DrawTextureShader();
	}

	//mWorldBlock0->DrawTextureShader();
	//mWorldBlock1->DrawTextureShader();
	//mWorldBlock2->DrawTextureShader();
	//mWorldBlock3->DrawTextureShader();
	//mWorldBlock4->DrawTextureShader();
	//mWorldBlock5->DrawTextureShader();
	//mWorldBlock6->DrawTextureShader();
	//mWorldBlock7->DrawTextureShader();
	//mWorldBlock8->DrawTextureShader();

	Renderer::CheckForErrors();
	Renderer::SetShader(oldShader);
	Renderer::CheckForErrors();

	glDisable(GL_BLEND);


	// Restore previous shader
	Renderer::SetShader((ShaderType)prevShader);

	Renderer::EndFrame();
}

//WorldBlock* World::getWorldBlock() const {
//	return mWorldBlock1;
//}

World* World::getWorldInstance() {
	if (worldInstance == NULL)
	{
		worldInstance = new World();
	}

	return worldInstance;
}



World::World() {

	mCharacterPosition = vec3(3.0f, 5.0f, 20.0f);

	CenterBlock = vec2(0, 0);
	// Neighbors = getNeighbors(CenterBlock);
	mWorldBlock.push_back(new WorldBlock(vec2(0, 0)));
	
	

	//mWorldBlock0 = new WorldBlock(vec2(0, 0));
	//mWorldBlock1 = new WorldBlock(vec2(1, 0));
	//mWorldBlock2 = new WorldBlock(vec2(-1, 0));
	//mWorldBlock3 = new WorldBlock(vec2(1, 1));
	//mWorldBlock4 = new WorldBlock(vec2(0, 1));
	//mWorldBlock5 = new WorldBlock(vec2(-1, 1));
	//mWorldBlock6 = new WorldBlock(vec2(-1, -1));
	//mWorldBlock7 = new WorldBlock(vec2(0, -1));
	//mWorldBlock8 = new WorldBlock(vec2(1, -1));

	//mWorldBlock.insert()



	// Setup Camera
	//mCamera.push_back(new FirstPersonCamera(vec3(3.0f, 5.0f, 20.0f)));
	mCamera.push_back(new FirstPersonCamera(mCharacterPosition));
	/*mCamera.push_back(new StaticCamera(vec3(3.0f, 30.0f, 5.0f), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f)));
	mCamera.push_back(new StaticCamera(vec3(0.5f, 0.5f, 5.0f), vec3(0.0f, 0.5f, 0.0f), vec3(0.0f, 1.0f, 0.0f)));*/
	mCurrentCamera = 0;

#if defined(PLATFORM_OSX)
	//    int billboardTextureID = TextureLoader::LoadTexture("Textures/BillboardTest.bmp");
	int billboardTextureID = TextureLoader::LoadTexture("Textures/Particle.png");
#else
	//    int billboardTextureID = TextureLoader::LoadTexture("../Assets/Textures/BillboardTest.bmp");
	int billboardTextureID = TextureLoader::LoadTexture("../Assets/Textures/Particle.png");
#endif
	assert(billboardTextureID != 0);

	mpBillboardList = new BillboardList(2048, billboardTextureID);
}


World::~World() {
	//delete mWorldBlock0;
	//delete mWorldBlock1;
	//delete mWorldBlock2;
	//delete mWorldBlock3;
	//delete mWorldBlock4;
	//delete mWorldBlock5;
	//delete mWorldBlock6;
	//delete mWorldBlock7;
	//delete mWorldBlock8;
};


void World::AddParticleDescriptor(ParticleDescriptor* particleDescriptor)
{
	mParticleDescriptorList.push_back(particleDescriptor);
}


Animation* World::FindAnimation(ci_string animName)
{
	for (std::vector<Animation*>::iterator it = mAnimation.begin(); it < mAnimation.end(); ++it)
	{
		if ((*it)->GetName() == animName)
		{
			return *it;
		}
	}
	return nullptr;
}

AnimationKey* World::FindAnimationKey(ci_string keyName)
{
	for (std::vector<AnimationKey*>::iterator it = mAnimationKey.begin(); it < mAnimationKey.end(); ++it)
	{
		if ((*it)->GetName() == keyName)
		{
			return *it;
		}
	}
	return nullptr;
}

ParticleDescriptor* World::FindParticleDescriptor(ci_string name)
{
	for (std::vector<ParticleDescriptor*>::iterator it = mParticleDescriptorList.begin(); it < mParticleDescriptorList.end(); ++it)
	{
		if ((*it)->GetName() == name)
		{
			return *it;
		}
	}
	return nullptr;
}


Camera* World::GetCurrentCamera() const
{
	return mCamera[mCurrentCamera];
}

LightSource const World::getLightSourceAt(int index) {
	return *(lightSource[index]);
}

void World::AddParticleSystem(ParticleSystem* particleSystem)
{
	mParticleSystemList.push_back(particleSystem);
}

void World::AddBillboard(Billboard* b)
{
	mpBillboardList->AddBillboard(b);
}

void World::RemoveBillboard(Billboard* b)
{
	mpBillboardList->RemoveBillboard(b);
}


void World::checkNeighbors() {
	mNeighbors.clear();


	mNeighbors.push_back(vec2(CenterBlock.x - 1, CenterBlock.y + 1));	// upper left
	mNeighbors.push_back(vec2(CenterBlock.x, CenterBlock.y + 1));		// upper mid
	mNeighbors.push_back(vec2(CenterBlock.x + 1, CenterBlock.y + 1));	// upper right
	mNeighbors.push_back(vec2(CenterBlock.x - 1, CenterBlock.y));		// level left
	mNeighbors.push_back(vec2(CenterBlock.x + 1, CenterBlock.y));		// level right
	mNeighbors.push_back(vec2(CenterBlock.x - 1, CenterBlock.y - 1));	// lower left
	mNeighbors.push_back(vec2(CenterBlock.x, CenterBlock.y - 1));		// lower mid
	mNeighbors.push_back(vec2(CenterBlock.x + 1, CenterBlock.y - 1));	// lower right
	assert(mNeighbors.size() == 8);

	// generate the world blocks that were not called before
	int dIndex = 0;
	for (int i = 0; i < 8; i++) {
		bool isNew = true;
		for (int j = 0; j < mWorldBlock.size(); j++) {
			if (mWorldBlock[j]->getWorldBlockCoor() == mNeighbors[i]) {
				DisplayedWBIndex[dIndex++] = j;
				isNew = false;
				break;
			}
				
		}
		if (isNew) {
			DisplayedWBIndex[dIndex++] = mWorldBlock.size();
			WorldBlock* newWorldBlock = new WorldBlock(mNeighbors[i]);
			setupWorldBlock(newWorldBlock);
			mWorldBlock.push_back(newWorldBlock);
		}
			//mWorldBlock.push_back(new WorldBlock(mNeighbors[i]));
	}

	for (int i = 0; i < mWorldBlock.size(); i++) {
		if (mWorldBlock[i]->getWorldBlockCoor() == CenterBlock) {
			DisplayedWBIndex[8] = i;
			break;
		}
	}

}