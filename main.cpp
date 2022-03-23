// Dear ImGui: standalone example application for GLFW + OpenGL 3, using programmable pipeline
// (GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs
#include "renderer3D.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GL/glew.h>
#include <stdio.h>
#include <stdlib.h>
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h> // Will drag system OpenGL headers
#include "TextEditor.h"
#include <iostream>
#include <sstream>
#include "ImGuizmo/ImGuizmo.h"


// [Win32] Our example includes a copy of glfw3.lib pre-compiled with VS2010 to maximize ease of testing and compatibility with old VS compilers.
// To link with VS2010-era libraries, VS2015+ requires linking with legacy_stdio_definitions.lib, which we do using this pragma.
// Your own project should not be affected, as you are likely to link with a newer binary of GLFW that is adequate for your version of Visual Studio.
#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

int main(int, char**)
{


    
    // Setup window
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100
    const char* glsl_version = "#version 100";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
    // GL 3.2 + GLSL 150
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

    // Create window with graphics context
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Dear ImGui Shader Editor", NULL, NULL);
    if (window == NULL)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Setup glew
    GLenum res = glewInit();
    if (res != GLEW_OK) {
        fprintf(stderr, "Error: %s\n", glewGetErrorString(res));
        return 1;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != NULL);


    // Our state
    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(33.0 / 255.0, 33.0 / 255.0, 35.0 / 255.0, 1.00f);
    ImVec4 muted_color = ImVec4(40.0 / 255.0, 40.0 / 255.0, 45.0 / 255.0, 1.00f);
    ImVec4 primary_color = ImVec4(75.0 / 255.0, 128.0 / 255.0, 202.0 / 255.0, 1.00f);
    ImVec4 active_color = ImVec4(180.0 / 255.0, 82.0 / 255.0, 82.0 / 255.0, 1.00f);


    ImVec2 size(800, 600);
    float azimuth = 45;
    float elevation = 45;
    float zoom = 2;

    glm::mat4 rotation(1);
    rotation = glm::rotate(rotation, azimuth * 0.01f, glm::vec3(0, -1, 0));
    rotation = glm::rotate(rotation, elevation * 0.01f, glm::vec3(-1, 0, 0));
    glm::vec4 newCamPosition(0, 0, zoom, 1);
    newCamPosition = rotation * newCamPosition;
    glm::vec3 cameraPosition;
    cameraPosition = newCamPosition;
    const char* fshaderfilepath = "./shaders/fshader.glsl";
    const char* vshaderfilepath = "./shaders/vshader.glsl";
    static char albedoPath[256] = "textures/Noises/1.png";
    static char normalPath[256] = "textures/Noises/1_Normal.png";        
    Renderer3D renderer3D(size, cameraPosition, "./models/bigGrid.obj", fshaderfilepath, vshaderfilepath);
    renderer3D.SetAlbedo(albedoPath);
    renderer3D.SetNormal(normalPath);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);



    //TEXT EDITORS LANG
	auto lang = TextEditor::LanguageDefinition::GLSL();

	// set your own known preprocessor symbols...
	static const char* ppnames[] = { "NULL", "assert" };
	// ... and their corresponding values
	static const char* ppvalues[] = { 
		"#define NULL ((void*)0)", 
		" #define assert(expression) (void)(                                                  \n"
        "    (!!(expression)) ||                                                              \n"
        "    (_wassert(_CRT_WIDE(#expression), _CRT_WIDE(__FILE__), (unsigned)(__LINE__)), 0) \n"
        " )"
		};

	for (int i = 0; i < sizeof(ppnames) / sizeof(ppnames[0]); ++i)
	{
		TextEditor::Identifier id;
		id.mDeclaration = ppvalues[i];
		lang.mPreprocIdentifiers.insert(std::make_pair(std::string(ppnames[i]), id));
	}

	// set your own identifiers
	static const char* identifiers[] = {
        "abs","acos","acosh","all","any","asin","asinh","atan","atanh","atomicAdd","atomicAnd","atomicCompSwap",
        "atomicCounter","atomicCounterDecrement","atomicCounterIncrement","atomicExchange","atomicMax","atomicMin",
        "atomicOr","atomicXor","barrier","bitCount","bitfieldExtract","bitfieldInsert","bitfieldReverse","ceil","clamp",
        "cos","cosh","cross","degrees","determinant","dFdx","dFdxCoarse","dFdxFine","dFdy","dFdyCoarse","dFdyFine",
        "distance","dot","EmitStreamVertex","EmitVertex","EndPrimitive","equal","exp","exp2","faceforward","findLSB",
        "findMSB","floatBitsToInt","floatBitsToUint","floor","fma","fract","frexp","fwidth","fwidthCoarse","fwidthFine",
        "greaterThan","greaterThanEqual","groupMemoryBarrier","imageAtomicAdd","imageAtomicAnd","imageAtomicCompSwap",
        "imageAtomicExchange","imageAtomicMax","imageAtomicMin","imageAtomicOr","imageAtomicXor","imageLoad",
        "imageSamples","imageSize","imageStore","imulExtended","intBitsToFloat","interpolateAtCentroid",
        "interpolateAtOffset","interpolateAtSample","inverse","inversesqrt","isinf","isnan","ldexp","length","lessThan",
        "lessThanEqual","log","log2","matrixCompMult","max","memoryBarrier","memoryBarrierAtomicCounter",
        "memoryBarrierBuffer","memoryBarrierImage","memoryBarrierShared","min","mix","mod","modf","noise","noise1",
        "noise2","noise3","noise4","normalize","not","notEqual","outerProduct","packDouble2x32","packHalf2x16",
        "packSnorm2x16","packSnorm4x8","packUnorm","packUnorm2x16","packUnorm4x8","pow","radians","reflect","refract",
        "removedTypes","round","roundEven","sign","sin","sinh","smoothstep","sqrt","step","tan","tanh","texelFetch",
        "texelFetchOffset","texture","textureGather","textureGatherOffset","textureGatherOffsets","textureGrad",
        "textureGradOffset","textureLod","textureLodOffset","textureOffset","textureProj","textureProjGrad",
        "textureProjGradOffset","textureProjLod","textureProjLodOffset","textureProjOffset","textureQueryLevels",
        "textureQueryLod","textureSamples","textureSize","transpose","trunc","uaddCarry","uintBitsToFloat",
        "umulExtended","unpackDouble2x32","unpackHalf2x16","unpackSnorm2x16","unpackSnorm4x8","unpackUnorm",
        "unpackUnorm2x16","unpackUnorm4x8","usubBorrow","in","uniform","out","layout"
     };

	for (int i = 0; i < sizeof(identifiers) / sizeof(identifiers[0]); ++i)
	{
		TextEditor::Identifier id;
		id.mDeclaration = std::string(identifiers[i]);
		lang.mIdentifiers.insert(std::make_pair(std::string(identifiers[i]), id));
	}

    const char* keywords[] = {
        "vec2",
        "vec3",
        "vec4",
        "mat4",
        "sampler2D"
     };

    for (int i = 0; i < 5; ++i) {
		lang.mKeywords.insert(std::string(keywords[i]));
	}


    //TEXT EDITORS OBJECTS
    TextEditor editorFShader;
	editorFShader.SetLanguageDefinition(lang);
    std::fstream fShaderFile(fshaderfilepath);
    std::stringstream ffilestream;
    ffilestream << fShaderFile.rdbuf();
    editorFShader.SetText(ffilestream.str());
    fShaderFile.close();

	//editorFShader.SetPalette(TextEditor::GetLightPalette());

	// error markers
	TextEditor::ErrorMarkers fsmarkers;
	//fsmarkers.insert(std::make_pair<int, std::string>(6, "Example error here:\nInclude file not found: \"TextEditor.h\""));
    editorFShader.SetErrorMarkers(fsmarkers);

    TextEditor editorVShader;
	editorVShader.SetLanguageDefinition(lang);
    std::fstream vShaderFile(vshaderfilepath);
    std::stringstream vfilestream;
    vfilestream << vShaderFile.rdbuf();
    editorVShader.SetText(vfilestream.str());
    vShaderFile.close();
    

	//editorVShader.SetPalette(TextEditor::GetLightPalette());

	// error markers
	TextEditor::ErrorMarkers vsmarkers;
	editorVShader.SetErrorMarkers(vsmarkers);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_Border, clear_color);
    ImGui::PushStyleColor(ImGuiCol_Separator, clear_color);
    ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, clear_color);
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, muted_color);
    ImGui::PushStyleColor(ImGuiCol_Button, muted_color);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, muted_color);
    ImGui::PushStyleColor(ImGuiCol_Button, primary_color);
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, primary_color);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, active_color);
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, active_color);
    
        
    //TIME
    float time = 0;
    float deltaTime = 0;

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);

        
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGuizmo::BeginFrame();
        ImGuizmo::Enable(true);

        

        // FRAGMENT SHADER EDITOR
        {
            auto cpos = editorFShader.GetCursorPosition();
            ImGui::Begin("Fragment Shader", nullptr,
             ImGuiWindowFlags_NoTitleBar | 
             ImGuiWindowFlags_NoDecoration | 
             ImGuiWindowFlags_NoResize |
             ImGuiWindowFlags_NoBackground);
            ImGui::SetWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);


            if (ImGui::Button("Compile")) {
                std::string error = renderer3D.SetFShader(editorFShader.GetText());
                fsmarkers.clear();
                if (error.compare("") != 0) {
                    int a, line, c;
                    char message[1024];
                    sscanf(error.c_str(), "%d(%d) : %[^\n]", &a, &line, &message);
                    std::string strmsg(message);
                    fsmarkers.insert(std::make_pair<int, std::string>((int)line, (std::string)strmsg));
                    printf("AFTER\n");
                } else {
                    fShaderFile.open(fshaderfilepath);
                    fShaderFile << editorFShader.GetText();
                    fShaderFile.close();
                }
                editorFShader.SetErrorMarkers(fsmarkers);
            }

            editorFShader.Render("TextEditor");

            //ImGui::Text("%6d/%-6d %6d lines  | %s | %s | %s | %s", cpos.mLine + 1, cpos.mColumn + 1, editorFShader.GetTotalLines(),
			//editorFShader.IsOverwrite() ? "Ovr" : "Ins",
			//editorFShader.CanUndo() ? "*" : " ",
			//editorFShader.GetLanguageDefinition().mName.c_str(), fshaderfile);


            ImGui::End();
        }

        // VERTEX SHADER EDITOR
        {
            auto cpos = editorVShader.GetCursorPosition();
            ImGui::Begin("Vertex Shader", nullptr,
             ImGuiWindowFlags_NoTitleBar | 
             ImGuiWindowFlags_NoDecoration | 
             ImGuiWindowFlags_NoResize |
             ImGuiWindowFlags_NoBackground);
            ImGui::SetWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);

            if (ImGui::Button("Compile")) {
                vsmarkers.clear();
                std::string error = renderer3D.SetVShader(editorVShader.GetText());
                if (error.compare("") != 0) {
                    int a, line, c;
                    char message[1024];
                    sscanf(error.c_str(), "%d(%d) : %[^\n]", &a, &line, &message);
                    std::string strmsg(message);
                    vsmarkers.insert(std::make_pair<int, std::string>((int)line, (std::string)strmsg));
                    printf("AFTER\n");
                } else {
                    vShaderFile.open(vshaderfilepath);
                    vShaderFile << editorVShader.GetText();
                    vShaderFile.close();
                }
                editorVShader.SetErrorMarkers(vsmarkers);
            }

            //ImGui::Text("%6d/%-6d %6d lines  | %s | %s | %s | %s", cpos.mLine + 1, cpos.mColumn + 1, editorFShader.GetTotalLines(),
			//editorFShader.IsOverwrite() ? "Ovr" : "Ins",
			//editorFShader.CanUndo() ? "*" : " ",
			//editorFShader.GetLanguageDefinition().mName.c_str(), fshaderfile);

            editorVShader.Render("TextEditor");

            ImGui::End();
        }

        // MODEL VIEWER
        {
            ImGui::Begin("Model Viewer", nullptr, 
             ImGuiWindowFlags_NoTitleBar | 
             ImGuiWindowFlags_NoDecoration | 
             ImGuiWindowFlags_NoResize |
             ImGuiWindowFlags_NoBackground);

            static bool validInput = false;
            if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                validInput = true;
            }

            float currentAzimuth = azimuth;
            float currentElevation = elevation;

            if (validInput) {
                ImVec2 currentdelta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left);

                currentAzimuth = azimuth + currentdelta.x;
                currentElevation = std::min((float)140, std::max((float)-140, elevation + currentdelta.y));


                if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
                    azimuth = currentAzimuth;
                    elevation = currentElevation;
                    validInput = false;
                } 
        
                


                glm::mat4 rotation(1);
                rotation = glm::rotate(rotation, currentAzimuth * 0.01f, glm::vec3(0, -1, 0));
                rotation = glm::rotate(rotation, currentElevation * 0.01f, glm::vec3(-1, 0, 0));
                glm::vec4 newCamPosition(0, 0, zoom, 1);
                newCamPosition = rotation * newCamPosition;
                cameraPosition = newCamPosition;
            }

            if (ImGui::IsWindowHovered() && ImGui::GetIO().MouseWheel) {
                zoom += (ImGui::GetIO().MouseWheel * 0.1f) * zoom;
                zoom = std::min(100.0f, std::max(0.2f, zoom));


                glm::mat4 rotation(1);
                rotation = glm::rotate(rotation, currentAzimuth * 0.01f, glm::vec3(0, -1, 0));
                rotation = glm::rotate(rotation, currentElevation * 0.01f, glm::vec3(-1, 0, 0));
                glm::vec4 newCamPosition(0, 0, zoom, 1);
                newCamPosition = rotation * newCamPosition;
                cameraPosition = newCamPosition;
            }


            renderer3D.Draw(ImVec2(ImGui::GetWindowSize().x - 16, ImGui::GetWindowSize().y - 16), clear_color, deltaTime, time);
            ImGui::SetCursorPos(ImVec2(20, 20));
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

            ImGuizmo::SetDrawlist();
			ImGuizmo::SetRect(ImGui::GetWindowPos().x + ImGui::GetWindowSize().x - 150, ImGui::GetWindowPos().y + 50, 100, 100 * ImGui::GetWindowSize().y / ImGui::GetWindowSize().x);
            ImGuizmo::SetGizmoSizeClipSpace(1);
            ImGuizmo::SetOrthographic(true);

            const glm::mat4 matrix(1);
            ImGuizmo::Manipulate((float*)&renderer3D.getViewMatrix()[0][0], (float*)& renderer3D.getProjectionMatrix()[0][0], ImGuizmo::OPERATION::TRANSLATE, ImGuizmo::MODE::WORLD, (float*)& matrix[0][0]);
            //ImGuizmo::DrawCubes((float*)&renderer3D.getViewMatrix()[0][0], (float*)& renderer3D.getProjectionMatrix()[0][0], (float*)& matrix[0][0], 1);
   
           
            ImGui::End();
        }

        {//DEMO
            ImGui::ShowDemoWindow();
        }

        {
            ImGui::Begin("Set Path");

            if (ImGui::InputText("Albedo path", albedoPath, 256, ImGuiInputTextFlags_EnterReturnsTrue)) {
                renderer3D.SetAlbedo(albedoPath);
            }

            if (ImGui::InputText("Normap path", normalPath, 256, ImGuiInputTextFlags_EnterReturnsTrue)) {
                renderer3D.SetNormal(normalPath);
            }

            static char screenPath[256] = "screenshots/screen.bmp";
            if (ImGui::InputText("Screenshot", screenPath, 256, ImGuiInputTextFlags_EnterReturnsTrue)) {
                renderer3D.Screenshot(screenPath);
            }

            ImGui::End();
        }


        deltaTime = 1.0f / (float)ImGui::GetIO().Framerate;
        time += deltaTime;

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();


    return 0;
}
