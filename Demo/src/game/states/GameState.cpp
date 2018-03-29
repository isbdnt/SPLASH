#include "GameState.h"
#include "../objects/Block.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;


GameState::GameState(StateStack& stack)
: State(stack)
, m_cam(30.f, 1280.f / 720.f, 0.1f, 5000.f)
, m_camController(&m_cam)
, m_fpsText(&m_font, L"")
, m_debugCamText(&m_font, L"")
, m_flyCam(true)
{

	// Get the Application instance
	m_app = Application::getInstance();
	//m_scene = std::make_unique<Scene>(AABB(Vector3(-100.f, -100.f, -100.f), Vector3(100.f, 100.f, 100.f)));

	// Set up camera with controllers
	m_cam.setPosition(Vector3(1.5f, 1.f, -3.0f));
	
	// Set up the scene
	//m_scene->addSkybox(L"skybox_space_512.dds");
	// Add a directional light
	Vector3 color(1.0f, 1.0f, 1.0f);
 	Vector3 direction(0.4f, -0.6f, 1.0f);
	direction.Normalize();
	m_lights.setDirectionalLight(DirectionalLight(color, direction));

	// Set up HUD texts
	m_debugCamText.setPosition(Vector2(0.f, 20.f));
	// Add texts to the scene
	//m_scene->addText(&m_fpsText);
#ifdef _DEBUG
	//m_scene->addText(&m_debugCamText);
#endif

	m_app->getAPI()->setFaceCulling(GraphicsAPI::NO_CULLING);

	m_cubeModel = ModelFactory::CubeModel::Create(Vector3(.5f), &m_app->getResourceManager().getShaderSet<MaterialShader>());
	m_cubeModel->getMaterial()->setDiffuseTexture("missing.tga");
	//m_cubeModel = ModelFactory::PlaneModel::Create(Vector2(.5f), &m_app->getResourceManager().getShaderSet<MaterialShader>());

	Entity e;
	e.addComponent<TestComponent>(13.37f);
	e.addComponent<TestTwoComponent>(13.f, 37.f);
	/*TestComponent c1(1);
	TestTwoComponent c2(2, 3);*/
	Logger::Log("c1 id: " + std::to_string(e.getComponent<TestComponent>()->getStaticID()));
	Logger::Log("c2 id: " + std::to_string(e.getComponent<TestTwoComponent>()->getStaticID()));

}

GameState::~GameState() {
}

// Process input for the state
bool GameState::processInput(float dt) {

	auto& kbTracker = m_app->getInput().getKbStateTracker();
	auto& kbState = m_app->getInput().getKeyboardState();

#ifdef _DEBUG
	// Toggle camera controller on 'F' key or 'Y' btn
	if (kbTracker.pressed.F)
		m_flyCam = !m_flyCam;
	// Add point light at camera pos
	if (kbTracker.pressed.E) {
		PointLight pl;
		pl.setColor(Vector3(Utils::rnd(), Utils::rnd(), Utils::rnd()));
		pl.setPosition(m_cam.getPosition());
		pl.setAttenuation(.0f, 0.1f, 0.02f);
		m_lights.addPointLight(pl);
	}
#endif

	if (kbState.G) {
		Vector3 color(1.0f, 1.0f, 1.0f);;
		m_lights.setDirectionalLight(DirectionalLight(color, m_cam.getDirection()));
	}

	// Update the camera controller from input devices
	if (m_flyCam)
		m_camController.update(dt);

	// Reload shaders
	if (kbState.R) {
		m_app->getResourceManager().reloadShader<MaterialShader>();
		Event e(Event::POTATO);
		m_app->onEvent(e);
	}


	return true;
}

void GameState::onEvent(Event& event) {
	Logger::Log("Recieved event: " + std::to_string(event.getType()));

	EventDispatcher::dispatch<WindowResizeEvent>(event, FUNC(&GameState::onResize));
}

bool GameState::onResize(WindowResizeEvent& event) {
	m_cam.resize(event.getWidth(), event.getHeight());
	////m_scene->resize(width, height);
	return true;
}

// Updates the state
bool GameState::update(float dt) {

	// Update HUD texts
	m_fpsText.setText(L"FPS: " + std::to_wstring(m_app->getFPS()));

	auto& camPos = m_cam.getPosition();
	m_debugCamText.setText(L"Camera @ " + Utils::vec3ToWStr(camPos));

	// Update camera in shaders
	m_app->getResourceManager().getShaderSet<MaterialShader>().updateCamera(m_cam);
	//m_app->getResourceManager().getShaderSet<MaterialShader>().updateLights(m_lights);

	return true;
}

// Renders the state
bool GameState::render(float dt) {

	// Clear back buffer
	m_app->getAPI()->clear({0.1f, 0.2f, 0.3f, 1.0f});

	// Draw the scene using deferred rendering
// 	m_scene->draw(dt, m_cam, m_level.get(), m_projHandler.get(), m_gamemode.get(), m_particleHandler.get());
	m_renderer.begin(&m_cam);
	m_renderer.setLightSetup(&m_lights);
	m_renderer.submit(m_cubeModel.get(), Matrix::Identity);
	m_renderer.submit(m_cubeModel.get(), Matrix::CreateTranslation(1.5f, 0.f, 0.f));
	m_renderer.submit(m_cubeModel.get(), Matrix::CreateTranslation(0.f, 1.5f, 0.f));
	m_renderer.end();
	m_renderer.present();

	// Draw HUD
	//m_scene->drawHUD();

	return true;
}
