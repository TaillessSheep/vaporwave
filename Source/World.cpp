#include "World.h"

#include "StaticCamera.h"
#include "FirstPersonCamera.h"
#include "ThirdPersonCamera.h"

#include "Renderer.h"
#include "CubeModel.h"
#include "SphereModel.h"
#include "Animation.h"
#include "Billboard.h"
#include <GLFW/glfw3.h>
#include "EventManager.h"
#include "TextureLoader.h"
#include "CubeObj.hpp"
#include "ParticleDescriptor.h"
#include "ParticleEmitter.h"
#include "ParticleSystem.h"
#include "SphereObj.hpp"
#include "LightSource.h"
#include "MainCharacter.hpp"
#include "SkyBox.hpp"
#include "Terrain/Terrain.h"
#include "WillMath.h"
//#include <openglut.h>

World* World::worldInstance;
const float World::WorldBlockSize = 224;

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
				SphereIndex = mModel.size();
				SphereObj* sphere = new SphereObj();
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
            else if (result == "object"){
				assert(mBuildingModel == nullptr);
                CubeObj* cube = new CubeObj();
                cube->Load(iss);
                mModel.push_back(cube);
				mBuildingModel = cube;
            }
            else if (result == "maincharacter"){
				mCharater = new MainCharacter();
                mCharater->Load(iss);
                //mModel.push_back(mC);
				
            }
			else if (result == "skybox") {
				mskybox = new SkyBox();
				mskybox->Load(iss);
			}
			else if (result == "terrain") {
				mTerrain = new Terrain();
				mTerrain->Load(iss);
				mModel.push_back(mTerrain);
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


	setupWorldBlock(mWorldBlock[0]);
	mBuildingModel->getCornerPoint(cornerPoint);

	checkNeighbors();

	// Building colision
	vector<vec3> cbCorner;
	vec3 axisLength;
	vec3 centerPoint;
	mat4 buildingModelMatrix = mBuildingModel->GetWorldMatrix();
	mcPositionInitial = mcPosition;
	mCamera[mCurrentCamera]->Update(0.1);
}

void World::setupWorldBlock(WorldBlock* WB) {
	
	WB->setAnimationKey(mAnimationKey);
	WB->setAnimation(mAnimation);
	WB->setParticleSystemList(mParticleSystemList);
	WB->setParticleDescriptorList(mParticleDescriptorList);

	WB->setModel(mModel);
	
	WB->setLightSource(lightSource);
	WB->setBillboardList(mpBillboardList);

	WB->setSphereIndex(SphereIndex);

}

void World::Update(float dt) {

	// update charater --------------------------------------------------------------
	// Prevent from having the camera move only when the cursor is within the windows

	// The Camera moves based on the User inputs
	// - You can access the mouse motion with EventManager::GetMouseMotionXY()
	// - For mapping A S D W, you can look in WorldBlock.cpp for an example of accessing key states
	// - Don't forget to use dt to control the speed of the camera motion
	//Camera* cam = GetCurrentCamera();
	//if (dynamic_cast<ThirdPersonCamera*>(cam) != nullptr)
	//	dynamic_cast<ThirdPersonCamera*>(GetCurrentCamera())->setleftKeyPressed(false);
	if (glfwGetMouseButton(EventManager::GetWindow(), GLFW_MOUSE_BUTTON_2) == GLFW_PRESS) {
		EventManager::DisableMouseCursor();
		ThirdPersonCamera* TPC = static_cast<ThirdPersonCamera*>(mCamera[1]);
		TPC->setleftKeyPressed(false);
		// Mouse motion to get the variation in angle
		mHorizontalAngle -= EventManager::GetMouseMotionX() * mAngularSpeed * dt;
		mVerticalAngle -= EventManager::GetMouseMotionY() * mAngularSpeed * dt;
	}
	else if(glfwGetMouseButton(EventManager::GetWindow(), GLFW_MOUSE_BUTTON_1) == GLFW_PRESS && 
		mCurrentCamera == 1) {
		EventManager::DisableMouseCursor();
		ThirdPersonCamera* TPC = static_cast<ThirdPersonCamera*>(mCamera[1]);
		TPC->setleftKeyPressed(true);
		float mCamHorizontalAngle = EventManager::GetMouseMotionX() * mAngularSpeed * dt;
		float mCamVerticalAngle = EventManager::GetMouseMotionY() * mAngularSpeed * dt;
		TPC->addExtraCamAngle(mCamHorizontalAngle, mCamVerticalAngle);
	}
	else
	{
		ThirdPersonCamera* TPC = static_cast<ThirdPersonCamera*>(mCamera[1]);
		TPC->setleftKeyPressed(false);
		EventManager::EnableMouseCursor();
	}

	//mouse left click and hold to change the camera position and angle

	// Clamp vertical angle to [-85, 85] degrees
	mVerticalAngle = std::max(-85.0f, std::min(85.0f, mVerticalAngle));
	if (mHorizontalAngle > 360)
	{
		mHorizontalAngle -= 360;
	}
	else if (mHorizontalAngle < -360)
	{
		mHorizontalAngle += 360;
	}

	float theta = radians(mHorizontalAngle);
	float phi = radians(mVerticalAngle);

	mcLookAt = vec3(cosf(phi)*cosf(theta), sinf(phi), -cosf(phi)*sinf(theta));
	vec3 groundNormal = vec3(0.0f, 1.0f, 0.0f);


	vec3 RelativeCoor = vec3(mcPosition.x - CenterBlock.x * World::WorldBlockSize , 0.0, mcPosition.z - CenterBlock.y * World::WorldBlockSize);

	float groundHight = 0;
	mTerrain->getHightAndNormal(RelativeCoor, groundHight, groundNormal);

	

	mcSideVector = glm::cross(mcLookAt, vec3(0.0f, 1.0f, 0.0f));
   	glm::normalize(mcSideVector);


	vec3 sDirection(0.0f);
	//bool SpacePressed = false;

	// A S D W for motion along the camera basis vectors
	// Forward
	if (glfwGetKey(EventManager::GetWindow(), GLFW_KEY_W) == GLFW_PRESS) 
	{
		sDirection += mcLookAt;
	}
	// Backward
	if (glfwGetKey(EventManager::GetWindow(), GLFW_KEY_S) == GLFW_PRESS)
	{

		sDirection -= mcLookAt;

	}
	// To the left
	if (glfwGetKey(EventManager::GetWindow(), GLFW_KEY_D) == GLFW_PRESS)
	{
		sDirection += mcSideVector;
	}
	// To the right
	if (glfwGetKey(EventManager::GetWindow(), GLFW_KEY_A) == GLFW_PRESS)
	{
		sDirection -= mcSideVector;
	}
	// Up
	if (glfwGetKey(EventManager::GetWindow(), GLFW_KEY_SPACE) == GLFW_PRESS)
	{

		sDirection += vec3(0, 1, 0);
	}
	else if ((mcPosition.y - mcRadius) > groundHight + 2)
	{
		sDirection += vec3(0, -0.1, 0);
	}
	// Speed
	float mSpeed = mCharacterDefaultSpeed * mCharacterSpeedUpRate;;


		


	// Ground collision
	if ((mcPosition.y - mcRadius) <= groundHight + 1 && dot(sDirection, groundNormal) < 0.0f) {

		vec3 mSideVector = cross(groundNormal, sDirection);
		sDirection = cross(mSideVector, groundNormal);
		sDirection = normalize(sDirection);
	}

	// Building colision
	vector<vec3> cbCorner;
	vec3 axisLength;
	vec3 centerPoint;
	//vector<vec3> doubleCheckNormals;
	vec3 firstNormal = vec3(0.0f);
	mat4 modelScalingMatrix = mBuildingModel->GetWorldMatrix();
	if (length(sDirection) > 0)
		for (int b = 0; b < mBuildingsMw.size(); b++) { // b for building
		
			// get the corner point for the current building in world space
			cbCorner.clear();
			for (int c = 0; c < 8; c++) { // c for corners	
				cbCorner.push_back(vec3(mBuildingsMw[b] * modelScalingMatrix * vec4(cornerPoint[c], 1.0f)));
				//diffVecSum += cbCorner[c] - mcPosition;
			}
			assert(cbCorner.size() == 8);
			// length of the building on x, y & z axis
			axisLength = cbCorner[2] - cbCorner[4];
			centerPoint = (cbCorner[2] + cbCorner[4]) / 2.0f;

			vec3 refVector = mcPosition - centerPoint;
			//vec3 nRefVector = normalize();
			normalize(refVector);
			//assert(length(refVector) > 1.1);
			refVector -= (mcRadius) * normalize(refVector);

		
			// not close to this building, continue the loop
			if (abs(refVector.x) > 0.5f*axisLength.x ||
				abs(refVector.y) > 0.5f*axisLength.y ||
				abs(refVector.z) > 0.5f*axisLength.z)
				continue;

			//if close to this building 
			//cout << "...(" << centerPoint.x << "," << centerPoint.y << "," << centerPoint.z << ")" << endl;
			refVector /= (0.5f*axisLength);

			int mIndex = 0;
			float max = abs(refVector.x);
			if (abs(refVector.y) > max) {
				max = abs(refVector.y);
				mIndex = 1;
			}
			if (abs(refVector.z) > max) {
				max = abs(refVector.z);
				mIndex = 2;
			}

			vec3 mNormal;
			if (mIndex == 0) {
				mNormal = vec3(1.0f, 0.0f, 0.0f);
				if (refVector.x < 0)
					mNormal *= -1;
			}
			else if (mIndex == 1) {
				mNormal = vec3(0.0f, 1.0f, 0.0f);
				if (refVector.y < 0)
					mNormal *= -1;
			}
			else {
				mNormal = vec3(0.0f, 0.0f, 1.0f);
				if (refVector.z < 0)
					mNormal *= -1;
			}

			if (dot(sDirection, mNormal) < 0.0f) {

				vec3 mSideVector = cross(mNormal, sDirection);
				sDirection = cross(mSideVector, mNormal);

				sDirection = normalize(sDirection);
			}

		}


	if (glfwGetKey(EventManager::GetWindow(), GLFW_KEY_P) == GLFW_PRESS)
		cout << "Pausing" << endl;
	if (isnan(sDirection.x)) {
		sDirection = vec3(0.0f);
	}
	else if (length(sDirection) > 0) {
		sDirection = normalize(sDirection);
	}

	if(sDirection.y<0)
		sDirection *= vec3(1.0f, 0.3f, 1.0f);

	// new charater position
	mcPosition += sDirection * dt * mSpeed;


	// Back to intial
	if (glfwGetKey(EventManager::GetWindow(), GLFW_KEY_O) == GLFW_PRESS)
	{
		if (glfwGetKey(EventManager::GetWindow(), GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS ||
			glfwGetKey(EventManager::GetWindow(), GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
			mcPosition = mcPositionInitial;
	}

	if (mcPosition.y - mcRadius <= groundHight)
		mcPosition.y = groundHight + mcRadius;


	mCharater->Update(0.1);
	
	// update charater ends --------------------------------------------------------------




	// Update current Camera
	mCamera[mCurrentCamera]->Update(dt);


	// check for the center block
	int x = floor((mcPosition.x + WorldBlockSize/2) / WorldBlockSize);
	int z = floor((mcPosition.z + WorldBlockSize/2) / WorldBlockSize);
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
	
	//if(glfwgetMous)

	// Spacebar to change the shader
	if (glfwGetKey(EventManager::GetWindow(), GLFW_KEY_0) == GLFW_PRESS)
	{
		Renderer::SetShader(SHADER_SOLID_COLOR);
	}
	//else if (glfwGetKey(EventManager::GetWindow(), GLFW_KEY_9) == GLFW_PRESS)
	//{
	//	Renderer::SetShader(SHADER_BLUE);
	//}

	for (vector<ParticleSystem*>::iterator it = mParticleSystemList.begin(); it != mParticleSystemList.end(); ++it)
	{
		(*it)->Update(dt);
	}

	mpBillboardList->Update(dt);
	mcBillboardList->Update(dt);
	mcParticleSystem->Update(dt, true);
	mWorldBlock[0]->Update(dt);

	if (!mWorldBlock[DisplayedWBIndex[8]]->IsLightSphere()) {
		vec3 sPosition = mModel[SphereIndex]->GetPosition();
		mat4 offSet = mWorldBlock[DisplayedWBIndex[8]]->getWBOffsetMatrix();

		sPosition = vec3(offSet * vec4(sPosition, 1.0f));

		vec3 diffVec = sPosition - mcPosition;
		if (length(diffVec) < mcRadius) {
			mWorldBlock[DisplayedWBIndex[8]]->setIsLightSphere(true);
		}
	}
}


void World::Draw() {

	glClearColor(0.1f, 0.13f, 0.2f,0.0f);

	//first shader
	Renderer::BeginFrame();
	// Set shader to use
	glUseProgram(Renderer::GetShaderProgramID());
	Renderer::CheckForErrors();

	for (int i = 0; i < 9; i++) {
		mWorldBlock[DisplayedWBIndex[i]]->DrawCurrentShader();
	}
	//mWorldBlock[DisplayedWBIndex[8]]->DrawCurrentShader();
	if(mCurrentCamera != 0)
 		mCharater->Draw(mat4(1.0f));

	
	// path lines shader
	Renderer::CheckForErrors();
	unsigned int prevShader = Renderer::GetCurrentShader();

	Renderer::SetShader(SHADER_LIGHTSOURCE);
	glUseProgram(Renderer::GetShaderProgramID());
	Renderer::CheckForErrors();

	for (int i = 0; i < 9; i++) {
		//mWorldBlock[DisplayedWBIndex[i]]->DrawPathLinesShader();
		mWorldBlock[DisplayedWBIndex[i]]->DrawCurrentLightSources();
	}


	Renderer::CheckForErrors();

	Renderer::SetShader(SHADER_PATH_LINES);
	glUseProgram(Renderer::GetShaderProgramID());
	Renderer::CheckForErrors();

	for (int i = 0; i < 9; i++) {
		mWorldBlock[DisplayedWBIndex[i]]->DrawPathLinesShader();
	}

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

	mcBillboardList->Draw(mat4(1.0));
    Renderer::SetShader(SHADER_TEXTURED);

	Renderer::CheckForErrors();
	Renderer::SetShader(oldShader);
	Renderer::CheckForErrors();
    
    glDisable(GL_BLEND);
    Renderer::SetShader(SHADER_SKY);
    glUseProgram(Renderer::GetShaderProgramID());
  
	

    GLuint VPMatrixLocation = glGetUniformLocation(Renderer::GetShaderProgramID(), "ViewProjectionTransform");
    // Get a handle for our Transformation Matrices uniform
    GLuint ViewMatrixID = glGetUniformLocation(Renderer::GetShaderProgramID(), "ViewTransform");
    GLuint ProjMatrixID = glGetUniformLocation(Renderer::GetShaderProgramID(), "ProjectionTransform");
	mat4 VP = World::getWorldInstance()->GetCurrentCamera()->GetViewProjectionMatrix();
	glUniformMatrix4fv(VPMatrixLocation, 1, GL_FALSE, &VP[0][0]);
	
	// viewTransform
	//mat4 View = mCamera[mCurrentCamera]->GetViewMatrix();
	mat4 View = mat4(mat3(World::getWorldInstance()->GetCurrentCamera()->GetViewMatrix()));
	glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &View[0][0]);
	// projectionMatrix
	//mat4 Projection = mCamera[mCurrentCamera]->GetProjectionMatrix();
	mat4 Projection = World::getWorldInstance()->GetCurrentCamera()->GetProjectionMatrix();
	glUniformMatrix4fv(ProjMatrixID, 1, GL_FALSE, &Projection[0][0]);

	mskybox->Draw(mat4(1.0f));
    
   
	// Restore previous shader
	Renderer::SetShader((ShaderType)prevShader);
    glUseProgram(Renderer::GetShaderProgramID());
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

	//mcPosition = vec3(3.0f, 5.0f, 20.0f);
	mcPosition = vec3(0.0f, 100.0f, 0.0f);
	mcPositionInitial = mcPosition;
	mcLookAt = vec3(0.0f, 0.0f, -1.0f);

	CenterBlock = vec2(0, 0);
	// Neighbors = getNeighbors(CenterBlock);
	mWorldBlock.push_back(new WorldBlock(vec2(0, 0)));
	mWorldBlock[0]->setOnThis(true);
	oldCenterBlock = mWorldBlock[0];


	// Setup Camera
	//mCamera.push_back(new FirstPersonCamera(vec3(3.0f, 5.0f, 20.0f)));
	mCamera.push_back(new FirstPersonCamera(mcPosition, mcLookAt));
	mCamera.push_back(new ThirdPersonCamera(mcPosition,mcLookAt));

	glfwSetScrollCallback(EventManager::GetWindow(), scroll_callback);

	//mCamera.push_back
	//mCamera.push_back(new StaticCamera(vec3(3.0f, 30.0f, 5.0f), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f)));
	//mCamera.push_back(new StaticCamera(vec3(0.5f, 0.5f, 5.0f), vec3(0.0f, 0.5f, 0.0f), vec3(0.0f, 1.0f, 0.0f)));
	mCurrentCamera = 0;

#if defined(PLATFORM_OSX)
	//    int billboardTextureID = TextureLoader::LoadTexture("Textures/BillboardTest.bmp");
	int billboardTextureID = TextureLoader::LoadTexture("Textures/Particle.png");
#else
	//    int billboardTextureID = TextureLoader::LoadTexture("../Assets/Textures/BillboardTest.bmp");
	int billboardTextureID = TextureLoader::LoadTexture("../Assets/Textures/Particle3.png");
#endif
	assert(billboardTextureID != 0);

	mpBillboardList = new BillboardList(2048, billboardTextureID);


	int mcBillboardTextureID = TextureLoader::LoadTexture("../Assets/Textures/Particle3.png");
	assert(mcBillboardTextureID != 0);

	mcBillboardList = new BillboardList(2048, mcBillboardTextureID);


	//glutMouseWheelFunc(mouseWheel);


}

void World::scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
	ThirdPersonCamera* mCam = static_cast<ThirdPersonCamera*> (World::getWorldInstance()->getTCP());
	mCam->setDistance(yoffset);
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
		for (int j = mWorldBlock.size() - 1; j >= 0; j--) {
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

	for (int i = mWorldBlock.size()-1; i >= 0; i--) {
		if (mWorldBlock[i]->getWorldBlockCoor() == CenterBlock) {
			DisplayedWBIndex[8] = i;
			oldCenterBlock->setOnThis(false);
			oldCenterBlock = mWorldBlock[i];
			oldCenterBlock->setOnThis(true);
			break;
		}
	}


	mBuildingsMw.clear();
	for (int i = 0; i < 9; i++) {
		mWorldBlock[DisplayedWBIndex[i]]->getBuildingsWorldMatrix(mBuildingsMw);
	}


}
