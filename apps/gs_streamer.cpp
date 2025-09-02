#include <args/args.hxx>
#include <spdlog/spdlog.h>

#include <OpenGLApp.h>
#include <SceneLoader.h>
#include <Windowing/GLFWWindow.h>
#include <GUI/ImGuiManager.h>
#include <GSRenderer.h>
#include <PostProcessing/Tonemapper.h>

#include <CameraAnimator.h>

#include <Streamers/VideoStreamer.h>
#include <Receivers/PoseReceiver.h>

using namespace quasar;

static std::shared_ptr<GaussianCloud> LoadGaussianCloud(const std::string& plyFilename, const bool importFullSH = true)
{
    GaussianCloud::Options options = {0};
#ifdef __ANDROID__
    options.importFullSH = false;
    options.exportFullSH = false;
#else
    options.importFullSH = importFullSH;
    options.exportFullSH = true;
#endif
    auto gaussianCloud = std::make_shared<GaussianCloud>(options);
    if (!gaussianCloud->ImportPly(plyFilename)) {
        spdlog::error("Error loading GaussianCloud!");
        return nullptr;
    }

    return gaussianCloud;
}

int main(int argc, char** argv) {
    Config config{};
    config.title = "GS Streamer";

    args::ArgumentParser parser(config.title);
    args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});
    args::Flag verbose(parser, "verbose", "Enable verbose logging", {'v', "verbose"});
    args::ValueFlag<std::string> sizeIn(parser, "size", "Resolution of renderer", {'s', "size"}, "1920x1080");
    args::ValueFlag<std::string> plyFileIn(parser, "ply", "Path to ply", {'i', "ply"}, "./test.ply");
    args::Flag novsync(parser, "novsync", "Disable VSync", {'V', "novsync"}, false);
    args::ValueFlag<bool> displayIn(parser, "display", "Show window", {'d', "display"}, true);
    args::ValueFlag<std::string> videoURLIn(parser, "video", "Video URL", {'c', "video-url"}, "127.0.0.1:12345");
    args::ValueFlag<std::string> poseURLIn(parser, "pose", "Pose URL", {'p', "pose-url"}, "0.0.0.0:54321");
    args::ValueFlag<int> targetBitrateIn(parser, "targetBitrate", "Target bitrate (Mbps)", {'b', "target-bitrate"}, 12);
    args::ValueFlag<bool> vrModeIn(parser, "vr", "Enable VR mode", {'r', "vr"}, false);
    args::Flag importFullSH(parser, "importFullSH", "Import full SH data from PLY", {'f', "fullsh"}, true);
    try {
        parser.ParseCLI(argc, argv);
    } catch (args::Help) {
        std::cout << parser;
        return 0;
    } catch (args::ParseError e) {
        std::cerr << e.what() << std::endl;
        std::cerr << parser;
        return 1;
    }

    if (verbose) {
        spdlog::set_level(spdlog::level::debug);
    }

    // Parse size
    std::string sizeStr = args::get(sizeIn);
    size_t pos = sizeStr.find('x');
    glm::uvec2 windowSize = glm::uvec2(std::stoi(sizeStr.substr(0, pos)), std::stoi(sizeStr.substr(pos + 1)));
    config.width = windowSize.x;
    config.height = windowSize.y;

    config.enableVSync = !args::get(novsync);
    config.showWindow = args::get(displayIn);

    std::string plyFile = args::get(plyFileIn);
    std::string videoURL = args::get(videoURLIn);
    std::string poseURL = args::get(poseURLIn);

    uint targetBitrate = args::get(targetBitrateIn);
    bool vrMode = args::get(vrModeIn);

    auto window = std::make_shared<GLFWWindow>(config);
    auto guiManager = std::make_shared<ImGuiManager>(window);

    config.window = window;
    config.guiManager = guiManager;

    OpenGLApp app(config);
    GSRenderer renderer(config);

    Scene scene;
    std::unique_ptr<Camera> camera;
    SceneLoader loader;
    if (vrMode) {
        auto vrCamera = std::make_unique<VRCamera>(windowSize.x / 2, windowSize.y);
        vrCamera->right.setViewMatrix(vrCamera->left.getViewMatrix());
        vrCamera->right.setProjectionMatrix(vrCamera->left.getProjectionMatrix());
        camera = std::move(vrCamera);
    }
    else {
        auto perspectiveCamera = std::make_unique<PerspectiveCamera>(windowSize.x, windowSize.y);
        camera = std::move(perspectiveCamera);
    }

    glm::vec3 initialPosition = camera->getPosition();

    VideoStreamer videoStreamerRT({
        .width = windowSize.x,
        .height = windowSize.y,
        .internalFormat = GL_SRGB8,
        .format = GL_RGB,
        .type = GL_UNSIGNED_BYTE,
        .wrapS = GL_CLAMP_TO_EDGE,
        .wrapT = GL_CLAMP_TO_EDGE,
        .minFilter = GL_LINEAR,
        .magFilter = GL_LINEAR,
    }, videoURL, config.targetFramerate, targetBitrate);

    PoseReceiver poseReceiver(camera.get(), poseURL);

    // Post processing
    Tonemapper tonemapper;
    tonemapper.enableTonemapping(false); // making this false essentially just copies the framebuffer to the screen

    // Load the given ply file
    auto gaussianCloud = LoadGaussianCloud(plyFile, importFullSH);
    if (!gaussianCloud) {
        spdlog::error("Error loading GaussianCloud");
        return false;
    }
    spdlog::info("Successfully loaded {}!", plyFile);

    bool paused = false;
    RenderStats renderStats;
    pose_id_t currentFramePoseID;
    guiManager->onRender([&](double now, double dt) {
        static bool showFPS = true;
        static bool showUI = true;

        ImGui::NewFrame();

        uint flags = 0;
        ImGui::BeginMainMenuBar();
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Exit", "ESC")) {
                window->close();
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("FPS", 0, &showFPS);
            ImGui::MenuItem("UI", 0, &showUI);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();

        if (showFPS) {
            ImGui::SetNextWindowPos(ImVec2(10, 40), ImGuiCond_FirstUseEver);
            flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar;
            ImGui::Begin("", 0, flags);
            ImGui::Text("%.1f FPS (%.3f ms/frame)", ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate);
            ImGui::End();
        }

        if (showUI) {
            ImGui::SetNextWindowSize(ImVec2(600, 500), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowPos(ImVec2(10, 90), ImGuiCond_FirstUseEver);
            ImGui::Begin(config.title.c_str(), &showUI);
            ImGui::Text("OpenGL Version: %s", glGetString(GL_VERSION));
            ImGui::Text("GPU: %s\n", glGetString(GL_RENDERER));

            if (renderStats.trianglesDrawn < 100000)
                ImGui::TextColored(ImVec4(0,1,0,1), "Triangles Drawn: %d", renderStats.trianglesDrawn);
            else if (renderStats.trianglesDrawn < 500000)
                ImGui::TextColored(ImVec4(1,1,0,1), "Triangles Drawn: %d", renderStats.trianglesDrawn);
            else
                ImGui::TextColored(ImVec4(1,0,0,1), "Triangles Drawn: %d", renderStats.trianglesDrawn);

            if (renderStats.drawCalls < 200)
                ImGui::TextColored(ImVec4(0,1,0,1), "Draw Calls: %d", renderStats.drawCalls);
            else if (renderStats.drawCalls < 500)
                ImGui::TextColored(ImVec4(1,1,0,1), "Draw Calls: %d", renderStats.drawCalls);
            else
                ImGui::TextColored(ImVec4(1,0,0,1), "Draw Calls: %d", renderStats.drawCalls);

            ImGui::Separator();

            glm::vec3 position = camera->getPosition();
            glm::vec3 rotation = camera->getRotationEuler();
            ImGui::BeginDisabled();
            ImGui::DragFloat3("Camera Position", (float*)&position);
            ImGui::DragFloat3("Camera Rotation", (float*)&rotation);
            ImGui::EndDisabled();

            ImGui::Separator();

            ImGui::Text("Video URL: %s", videoURL.c_str());
            ImGui::Text("Pose URL: %s", poseURL.c_str());

            ImGui::Separator();

            ImGui::TextColored(ImVec4(1,0.5,0,1), "Video Frame Rate: %.1f FPS (%.3f ms/frame)", videoStreamerRT.getFrameRate(), 1000.0f / videoStreamerRT.getFrameRate());
            ImGui::TextColored(ImVec4(0,0.5,0,1), "Time to copy frame: %.3f ms", videoStreamerRT.stats.transferTimeMs);
            ImGui::TextColored(ImVec4(0,0.5,0,1), "Time to encode frame: %.3f ms", videoStreamerRT.stats.encodeTimeMs);
            ImGui::TextColored(ImVec4(0,0.5,0,1), "Time to send frame: %.3f ms", videoStreamerRT.stats.sendTimeMs);

            ImGui::Separator();

            ImGui::Text("Remote Pose ID: %d", currentFramePoseID);

            ImGui::Separator();

            ImGui::Checkbox("Pause", &paused);

            ImGui::End();
        }
    });

    app.onResize([&](uint width, uint height) {
        windowSize = glm::uvec2(width, height);
        renderer.setWindowSize(windowSize.x, windowSize.y);

        if (vrMode) {
            auto vrCamera = static_cast<VRCamera*>(camera.get());
            vrCamera->left.setAspect(windowSize.x / 2, windowSize.y);
            vrCamera->right.setAspect(windowSize.x / 2, windowSize.y);
            vrCamera->updateProjectionMatrix();
        }
        else {
            auto perspectiveCamera = static_cast<PerspectiveCamera*>(camera.get());
            perspectiveCamera->setAspect(windowSize.x, windowSize.y);
            perspectiveCamera->updateProjectionMatrix();
        }
    });

    app.onRender([&](double now, double dt) {
        // Handle keyboard input
        auto keys = window->getKeys();
        if (keys.ESC_PRESSED) {
            window->close();
        }

        if (paused) {
            return;
        }

        // Update all animations
        scene.updateAnimations(dt);

        // Receive pose
        pose_id_t poseID = poseReceiver.receivePose();
        if (poseID != -1) {
            // Offset camera
            if (camera->isVR()) {
                auto* vrCamera = static_cast<VRCamera*>(camera.get());
                vrCamera->left.setPosition(vrCamera->left.getPosition() + initialPosition);
                vrCamera->right.setPosition(vrCamera->right.getPosition() + initialPosition);
                vrCamera->left.updateViewMatrix();
                vrCamera->right.updateViewMatrix();
            }
            else {
                auto* perspectiveCamera = static_cast<PerspectiveCamera*>(camera.get());
                perspectiveCamera->setPosition(perspectiveCamera->getPosition() + initialPosition);
                perspectiveCamera->updateViewMatrix();
            }

            renderStats = renderer.drawSplats(gaussianCloud, scene, *camera);

            // Restore camera position
            if (camera->isVR()) {
                auto* vrCamera = static_cast<VRCamera*>(camera.get());
                vrCamera->left.setPosition(vrCamera->left.getPosition() - initialPosition);
                vrCamera->right.setPosition(vrCamera->right.getPosition() - initialPosition);
                vrCamera->left.updateViewMatrix();
                vrCamera->right.updateViewMatrix();
            }
            else {
                auto* perspectiveCamera = static_cast<PerspectiveCamera*>(camera.get());
                perspectiveCamera->setPosition(perspectiveCamera->getPosition() - initialPosition);
                perspectiveCamera->updateViewMatrix();
            }

            // Copy rendered result to video render target
            tonemapper.drawToRenderTarget(renderer, videoStreamerRT);

            // Send video frame
            currentFramePoseID = poseID;
            videoStreamerRT.sendFrame(poseID);
        }

        if (config.showWindow) {
            tonemapper.drawToScreen(renderer);
        }
    });

    // Run app loop (blocking)
    app.run();

    spdlog::info("Please do CTRL-C to exit!");

    return 0;
}
