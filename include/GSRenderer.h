#ifndef GS_RENDERER_H
#define GS_RENDERER_H

#include <Renderers/OpenGLRenderer.h>
#include <RenderTargets/FrameRenderTarget.h>

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

private:
    bool useRgcSortOverride = false;
    bool isFramebufferSRGBEnabled = false;

    bool splatRendererInitialized = false;
    std::shared_ptr<SplatRenderer> splatRenderer;
};

} // namespace quasar

#endif // GS_RENDERER_H
