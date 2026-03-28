#ifndef GS_RENDERER_H
#define GS_RENDERER_H

#include <Renderers/OpenGLRenderer.h>
#include <RenderTargets/FrameRenderTarget.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>

#include <util.h>
#include <gaussiancloud.h>
#include <splatrenderer.h>

namespace quasar {

class GSRenderer : public OpenGLRenderer {
public:
    bool multiSampled = false;

    FrameRenderTarget frameRT;

    GSRenderer(const Config& config);
    ~GSRenderer() = default;

    virtual void setScreenShaderUniforms(const Shader& screenShader) override;

    virtual void resize(uint width, uint height) override;

    virtual void beginRendering() override;
    virtual void endRendering() override;

    RenderStats drawSplats(std::shared_ptr<GaussianCloud> gaussianCloud, const Scene& scene, const Camera& camera, uint32_t clearMask = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    // Model transform for the gaussian splat cloud
    glm::vec3 modelPosition = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 modelRotationDeg = glm::vec3(180.0f, 0.0f, 0.0f); // default: 180° around X to fix orientation
    glm::vec3 modelScale = glm::vec3(1.0f, 1.0f, 1.0f);

    glm::mat4 getModelMatrix() const {
        glm::mat4 T = glm::translate(glm::mat4(1.0f), modelPosition);
        glm::mat4 R = glm::eulerAngleXYZ(
            glm::radians(modelRotationDeg.x),
            glm::radians(modelRotationDeg.y),
            glm::radians(modelRotationDeg.z)
        );
        glm::mat4 S = glm::scale(glm::mat4(1.0f), modelScale);
        return T * R * S;
    }

private:
    bool useRgcSortOverride = false;
    bool isFramebufferSRGBEnabled = false;

    bool splatRendererInitialized = false;
    std::shared_ptr<SplatRenderer> splatRenderer;
};

} // namespace quasar

#endif // GS_RENDERER_H
