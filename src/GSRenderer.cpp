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
    screenShader.setTexture("screenColor", frameRT.colorTexture, 0);
    screenShader.setTexture("screenDepth", frameRT.depthStencilTexture, 1);
    screenShader.setTexture("screenNormals", frameRT.normalsTexture, 2);
    screenShader.setTexture("screenPositions", frameRT.normalsTexture, 3); // RenderTarget has no position buffer
    screenShader.setTexture("idTexture", frameRT.idTexture, 4);
}

RenderStats GSRenderer::drawSplats(std::shared_ptr<GaussianCloud> gaussianCloud, const Scene& scene, const Camera& camera, uint32_t clearMask) {
    RenderStats stats;
    if (!splatRendererInitialized && !splatRenderer->Init(gaussianCloud, isFramebufferSRGBEnabled, useRgcSortOverride)) {
        spdlog::error("Error initializing splat renderer!");
        return stats;
    }
    splatRendererInitialized = true;

    beginRendering();

    // Clear screen
    glClearColor(scene.backgroundColor.r, scene.backgroundColor.g, scene.backgroundColor.b, scene.backgroundColor.a);
    glClear(clearMask);

    glm::vec4 viewport;
    glm::mat4 projMat, cameraMat;
    if (camera.isVR()) {
        pipeline.rasterState.scissorTestEnabled = true;
        pipeline.apply();

        auto* vrCamera = static_cast<const VRCamera*>(&camera);

        cameraMat = vrCamera->left.getViewMatrixInverse();
        projMat = vrCamera->left.getProjectionMatrix();
        glm::vec2 nearFar(vrCamera->left.getNear(), vrCamera->left.getFar());

        // Left eye
        frameRT.setViewport({ 0, 0, width / 2, height });
        frameRT.setScissor({ 0, 0, width / 2, height });
        viewport = glm::vec4(0.0f, 0.0f, width / 2, height);
        splatRenderer->Sort(cameraMat, projMat, viewport, nearFar);
        splatRenderer->Render(cameraMat, projMat, viewport, nearFar);

        // Right eye
        cameraMat = vrCamera->right.getViewMatrixInverse();
        projMat = vrCamera->right.getProjectionMatrix();
        viewport = glm::vec4(width / 2, 0.0f, width / 2, height);

        frameRT.setViewport({ width / 2, 0, width / 2, height });
        frameRT.setScissor({ width / 2, 0, width / 2, height });
        splatRenderer->Sort(cameraMat, projMat, viewport, nearFar);
        splatRenderer->Render(cameraMat, projMat, viewport, nearFar);

        frameRT.setViewport({ 0, 0, width, height });
        frameRT.setScissor({ 0, 0, width, height });

        stats.trianglesDrawn = static_cast<uint>(gaussianCloud->GetNumGaussians()) * 2;
        stats.drawCalls = 2;
    }
    else {
        pipeline.apply();

        // Non-VR rendering
        frameRT.setViewport({ 0, 0, width, height });

        auto* perspectiveCamera = static_cast<const PerspectiveCamera*>(&camera);
        cameraMat = perspectiveCamera->getViewMatrixInverse();
        projMat = perspectiveCamera->getProjectionMatrix();
        viewport = glm::vec4(0.0f, 0.0f, width, height);
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
