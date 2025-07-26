#include <Cameras/PerspectiveCamera.h>
#include <Cameras/VRCamera.h>
#include <GSRenderer.h>

using namespace quasar;

GSRenderer::GSRenderer(const Config& config)
    : multiSampled(config.pipeline.multiSampleState.multiSampleEnabled)
    , frameRT({
        .width = config.width,
        .height = config.height,
        .multiSampled = false,
    })
    , splatRenderer(std::make_shared<SplatRenderer>())
    , OpenGLRenderer(config)
{
    // Splats need this specific alpha blending function
    pipeline.blendState.blendEnabled = true;
    pipeline.blendState.srcFactor = GL_ONE;
    pipeline.blendState.dstFactor = GL_ONE_MINUS_SRC_ALPHA;
    pipeline.blendState.blendEquation = GL_FUNC_ADD;
}

void GSRenderer::setScreenShaderUniforms(const Shader& screenShader) {
    // Set FrameRenderTarget texture uniforms
    screenShader.bind();
    screenShader.setTexture("screenColor", frameRT.colorBuffer, 0);
    screenShader.setTexture("screenDepth", frameRT.depthStencilBuffer, 1);
    screenShader.setTexture("screenNormals", frameRT.normalsBuffer, 2);
    screenShader.setTexture("screenPositions", frameRT.normalsBuffer, 3); // RenderTarget has no position buffer
    screenShader.setTexture("idBuffer", frameRT.idBuffer, 4);
}

RenderStats GSRenderer::drawSplats(std::shared_ptr<GaussianCloud> gaussianCloud, const Scene& scene, const Camera& camera, uint32_t clearMask) {
    RenderStats stats;
    if (!splatRendererInitialized && !splatRenderer->Init(gaussianCloud, isFramebufferSRGBEnabled, useRgcSortOverride)) {
        spdlog::error("Error initializing splat renderer!");
        return stats;
    }
    splatRendererInitialized = true;

    pipeline.apply();
    beginRendering();

    // Clear screen
    glClearColor(scene.backgroundColor.r, scene.backgroundColor.g, scene.backgroundColor.b, scene.backgroundColor.a);
    glClear(clearMask);

    // Render the splats
    if (camera.isVR()) {
        auto* vrCamera = static_cast<const VRCamera*>(&camera);

        pipeline.rasterState.scissorTestEnabled = true;

        // Left eye
        frameRT.setViewport(0, 0, width / 2, height);
        frameRT.setScissor(0, 0, width / 2, height);
        stats += drawSplats(gaussianCloud, scene, vrCamera->left, clearMask);

        // Right eye
        frameRT.setViewport(width / 2, 0, width / 2, height);
        frameRT.setScissor(width / 2, 0, width / 2, height);
        stats += drawSplats(gaussianCloud, scene, vrCamera->right, clearMask);
    }
    else {
        // Non-VR rendering
        frameRT.setViewport(0, 0, width, height);

        auto* perspectiveCamera = static_cast<const PerspectiveCamera*>(&camera);
        glm::mat4 cameraMat = perspectiveCamera->getViewMatrixInverse();
        glm::mat4 projMat = perspectiveCamera->getProjectionMatrix();
        glm::vec4 viewport(0.0f, 0.0f, (float)width, (float)height);
        glm::vec2 nearFar(perspectiveCamera->getNear(), perspectiveCamera->getFar());

        splatRenderer->Sort(cameraMat, projMat, viewport, nearFar);
        splatRenderer->Render(cameraMat, projMat, viewport, nearFar);

        stats.trianglesDrawn = static_cast<uint>(gaussianCloud->GetNumGaussians());
        stats.drawCalls = 1;
    }

    endRendering();
    return stats;
}

void GSRenderer::resize(uint width, uint height) {
    OpenGLRenderer::resize(width, height);
    frameRT.resize(width, height);
}

void GSRenderer::beginRendering() {
    frameRT.bind();
}

void GSRenderer::endRendering() {
    frameRT.unbind();
}
