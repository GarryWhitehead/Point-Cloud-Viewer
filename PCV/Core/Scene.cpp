#include "Scene.h"

#include "Core/Camera.h"
#include "Core/Engine.h"

namespace PCV
{

Scene::Scene(Engine& engine)
    : engine(engine)
{
}

Scene::~Scene()
{
}


void Scene::getVisibleRenderables(
    Frustum& frustum, std::vector<OEScene::VisibleCandidate>& renderables)
{
    size_t workSize = renderables.size();

    auto visibilityCheck = [&frustum, &renderables](const size_t curr_idx, const size_t chunkSize) {
        assert(curr_idx + chunkSize <= renderables.size());
        for (size_t idx = curr_idx; idx < curr_idx + chunkSize; ++idx)
        {
            Renderable* rend = renderables[idx].renderable;
            AABBox box {rend->instance->dimensions.min, rend->instance->dimensions.max};
            if (frustum.checkBoxPlaneIntersect(box))
            {
                rend->visibility |= Renderable::Visible::Render;
            }
        }
    };

    ThreadTaskSplitter splitWork {0, workSize, visibilityCheck};
    splitWork.run();
}

bool Scene::update(const double time)
{

    auto& objects = world.getObjectsList();
    auto& models = world.getModelGraph().getNodeList();

    // we create a temp container as we will be doing the visibility checks async
    // reserve more space than we need
    std::vector<VisibleCandidate> candRenderableObjs;
    candRenderableObjs.reserve(objects.size());

    // iterate through the model graph add as a possible candidate if active
    for (ModelGraph::Node* node : models)
    {
        // if the parent is inactive, then all its children are too
        if (!node->parent->isActive())
        {
            continue;
        }

        OEMaths::mat4f worldMat = node->world.worldMat;

        // the parent
        VisibleCandidate candidate = buildRendCandidate(node->parent, worldMat);
        candRenderableObjs.emplace_back(candidate);

        // and the children if any
        for (OEObject* child : node->children)
        {
            if (!child->isActive())
            {
                continue;
            }

            VisibleCandidate childCand = buildRendCandidate(child, worldMat);
            candRenderableObjs.emplace_back(childCand);
        }
    }

    // now for the lights. At the moment we iterate through the list of objects and find any that
    // have a light component. If they are active then these are added as a potential candiate
    // lighting source
    auto* lightManager = engine.getLightManager();

    std::vector<LightBase*> candLightObjs;
    candLightObjs.reserve(objects.size());

    for (OEObject* obj : objects)
    {
        if (!obj->isActive())
        {
            continue;
        }

        ObjectHandle lHandle = lightManager->getObjIndex(*obj);
        if (lHandle.valid())
        {
            candLightObjs.emplace_back(lightManager->getLight(lHandle));
        }
    }

    // prepare the camera frustum
    // update the camera matrices before constructing the fustrum
    Frustum frustum;
    camera->updateViewMatrix();
    frustum.projection(camera->getViewMatrix() * camera->getProjMatrix());

    // ============ visibility checks and culling ===================
    // first renderables - split work tasks and run async - Sets the visibility bit if passes
    // intersection test This will then be used to generate the render queue
    getVisibleRenderables(frustum, candRenderableObjs);

    // shadow culling tests
    // ** TODO **

    // and prepare the visible lighting list
    getVisibleLights(frustum, candLightObjs);

    // ============ render queue generation =========================
    std::vector<RenderableQueueInfo> queueRend;

    // key a count of the number of static and skinned models for later
    size_t staticModelCount = 0;
    size_t skinnedModelCount = 0;

    for (const VisibleCandidate& cand : candRenderableObjs)
    {
        Renderable* rend = cand.renderable;
        // only add visible renderables to the queue
        if (!rend->visibility.testBit(Renderable::Visible::Render))
        {
            continue;
        }

        if (rend->instance->variantBits.testBit(MeshInstance::Variant::HasSkin))
        {
            ++skinnedModelCount;
        }
        else
        {
            ++staticModelCount;
        }

        RenderableQueueInfo queueInfo;
        // we use the renderable data as it is, rather than waste time copying everything into
        // another struct. This method does mean that it is imperative that the data isnt destroyed
        // until the beginning of the next frame ad that the data isn't written too - we aren't
        // using guards though this might be required.
        queueInfo.renderableData = (void*) &rend;
        queueInfo.renderableHandle = this;
        queueInfo.renderFunction = GBufferFillPass::drawCallback;
        queueInfo.sortingKey = RenderQueue::createSortKey(
            RenderQueue::Layer::Default, rend->materialId, rend->instance->variantBits.getUint64());
        queueRend.emplace_back(queueInfo);
    }
    renderQueue.pushRenderables(queueRend, RenderQueue::Type::Colour);

    // ================== update ubos =================================
    // camera buffer is updated every frame as we expect this to change a lot
    updateCameraBuffer();

    // we also update the transforms every frame though could have a dirty flag
    updateTransformBuffer(candRenderableObjs, staticModelCount, skinnedModelCount);

    updateLightBuffer(candLightObjs);
    
    return true;
}

void OEScene::updateCameraBuffer()
{
    // update everything in the buffer
    OECamera::Ubo ubo;
    ubo.mvp = camera->getMvpMatrix();
    ubo.cameraPosition = camera->getPos();
    ubo.projection = camera->getProjMatrix();
    ubo.model = camera->getModelMatrix(); // this is just identity for now
    ubo.view = camera->getViewMatrix();
    ubo.zNear = camera->getZNear();
    ubo.zFar = camera->getZFar();

    driver.updateUbo(cameraUboName, sizeof(OECamera::Ubo), &ubo);
}

void Scene::setCurrentCamera(Camera* cam)
{
    assert(cam);
    camera = cam;
}

Camera* Scene::getCurrentCamera()
{
    return camera;
}


} // namespace OmegaEngine
