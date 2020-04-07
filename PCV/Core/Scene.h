#pragma once

#include "Rendering/RenderQueue.h"

#include <vector>

namespace VulkanAPI
{
class VkDriver;
}

namespace PCV
{

// forward decleartions
class Engine;
class Camera;

class Scene
{
public:
	
	/**
	* @brief A temp struct used to gather viable renderable objects data ready for visibilty checks
	* and passing to the render queue
	*/
	struct VisibleCandidate
	{
		Renderable* renderable;
		TransformInfo* transform;
        AABBox worldAABB;
        OEMaths::mat4f worldTransform;
	};

	Scene(Engine& engine);
	~Scene();

	bool update(const double time);

	void prepare();

	void updateCameraBuffer();

	Camera* getCurrentCamera();

	void getVisibleRenderables(Frustum& frustum, std::vector<VisibleCandidate>& renderables);
    
    VisibleCandidate buildRendCandidate(OEObject* obj, OEMaths::mat4f& worldMat);
    
    // ====== public functions for adding items to the scene ==============
    
    bool addSkybox(OESkybox* sb);
    
    void setCurrentCamera(OECamera* camera);
    
	friend class OERenderer;

private:

	/// per frame: all the renderables after visibility checks
    RenderQueue renderQueue;
    
	/// Current camera used by this scene. The 'world' holds the ownership of the cma
	Camera* camera;

	/// The world this scene is assocaited with
	Engine& engine;
};
}    // namespace OmegaEngine
