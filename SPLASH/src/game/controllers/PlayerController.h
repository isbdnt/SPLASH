#pragma once

class CameraController;
class Camera;
class Entity;

class Scene;
class Model;

// Will most likely be obselete once collision is implemented and projectiles can be destroyed that way.
struct Projectile {
	std::shared_ptr<Entity> projectile;
	float lifeTime = 0.f;
};

class PlayerController {
public:
	PlayerController(Camera* cam, Scene* scene);
	~PlayerController();

	void update(float dt);

	void setStartPosition(const glm::vec3& pos);
	void prepareUpdate();

	void processKeyboardInput(float dt);
	void processMouseInput(float dt);

	void updateCameraPosition(float alpha);

	std::shared_ptr<Entity> getEntity();

	void setProjectileModel(Model* model);
	std::shared_ptr<Entity> getCandle();
	void createCandle(Model* model);


	// Should be called at the start of the update loop and nowhere else
	void destroyOldProjectiles();
private:
	float m_movementSpeed = 20.f;
	float RUN_SPEED = 2.0f;

	// "Attached" camera
	CameraController* m_cam;
	
	Scene* m_scene;

	Model* m_projectileModel;
	
	std::vector<Projectile> m_projectiles;

	std::shared_ptr<Entity> m_player;

	std::shared_ptr<Entity> m_candle;

	// #netcodeNote not thread safe, might cause issues
	float m_yaw, m_pitch, m_roll;

	float m_lookSensitivityMouse = 0.1f;

	float m_projectileSpawnCounter = 0.f;

};