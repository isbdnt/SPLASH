#pragma once

// Ordered by name

class AiSystem;
class AnimationSystem;
class AnimationChangerSystem;
class AudioSystem;
class BeginEndFrameSystem;
class BoundingboxSubmitSystem;
class CandleHealthSystem;
class CandlePlacementSystem;
class CandleReignitionSystem;
class CollisionSystem;
class EndFrameSystem;
class EntityAdderSystem;
class EntityRemovalSystem;
class GameInputSystem;
class GUISubmitSystem;
class GunSystem;
class LevelGeneratorSystem;
class LifeTimeSystem;
class LightSystem;
class LightListSystem;
class MetaballSubmitSystem;
class ModelSubmitSystem;
class MovementPostCollisionSystem;
class MovementSystem;
class NetworkReceiverSystem;
class NetworkSenderSystem;
class OctreeAddRemoverSystem;
class ParticleSystem;
class PrepareUpdateSystem;
class ProjectileSystem;
class RenderImGuiSystem;
class SpeedLimitSystem;
class UpdateBoundingBoxSystem;

class NetworkReceiverSystem;

struct Systems {
	AiSystem* aiSystem = nullptr;
	AnimationSystem* animationSystem = nullptr;
	AnimationChangerSystem* animationChangerSystem = nullptr;
	AudioSystem* audioSystem = nullptr;
	BeginEndFrameSystem* beginEndFrameSystem = nullptr;
	BoundingboxSubmitSystem* boundingboxSubmitSystem = nullptr;
	CandleHealthSystem* candleHealthSystem = nullptr;
	CandlePlacementSystem* candlePlacementSystem = nullptr;
	CandleReignitionSystem* candleReignitionSystem = nullptr;
	CollisionSystem* collisionSystem = nullptr;
	EndFrameSystem* endFrameSystem = nullptr;
	EntityAdderSystem* entityAdderSystem = nullptr;
	EntityRemovalSystem* entityRemovalSystem = nullptr;
	GameInputSystem* gameInputSystem = nullptr;
	GUISubmitSystem* guiSubmitSystem = nullptr;
	GunSystem* gunSystem = nullptr;
	LevelGeneratorSystem* levelGeneratorSystem = nullptr;
	LifeTimeSystem* lifeTimeSystem = nullptr;
	LightSystem* lightSystem = nullptr;
	LightListSystem* lightListSystem = nullptr;
	MetaballSubmitSystem* metaballSubmitSystem = nullptr;
	ModelSubmitSystem* modelSubmitSystem = nullptr;
	MovementPostCollisionSystem* movementPostCollisionSystem = nullptr;
	MovementSystem* movementSystem = nullptr;
	NetworkReceiverSystem* networkReceiverSystem = nullptr;
	NetworkSenderSystem* networkSenderSystem = nullptr;
	OctreeAddRemoverSystem* octreeAddRemoverSystem = nullptr;
	ParticleSystem* particleSystem = nullptr;
	PrepareUpdateSystem* prepareUpdateSystem = nullptr;
	ProjectileSystem* projectileSystem = nullptr;
	RenderImGuiSystem* renderImGuiSystem = nullptr;
	SpeedLimitSystem* speedLimitSystem = nullptr;
	UpdateBoundingBoxSystem* updateBoundingBoxSystem = nullptr;
};