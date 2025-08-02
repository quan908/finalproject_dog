#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "SoundManager.hpp"
#include "GameObject.hpp"      
#include "ResourceManager.hpp" 

#include <iostream>
#include <vector>
#include <fstream>
#include "json.hpp"

using namespace XQ;
using namespace std;
using json = nlohmann::json;

bool checkCollision(const GameObject& one, const GameObject& two) {
    glm::vec2 oneMin = one.getMin();
    glm::vec2 oneMax = one.getMax();
    glm::vec2 twoMin = two.getMin();
    glm::vec2 twoMax = two.getMax();

    bool collisionX = oneMax.x >= twoMin.x && twoMax.x >= oneMin.x;
    bool collisionY = oneMax.y >= twoMin.y && twoMax.y >= oneMin.y;
    return collisionX && collisionY;
}

const char* vertexShaderSource = R"(
#version 330 core
layout (location=0) in vec3 aPos; layout (location=1) in vec3 aColor; layout (location=2) in vec2 atexCoord; layout (location=3) in vec3 aNormal;
out vec3 vertexColor; out vec2 texCoord; out vec3 normal; out vec3 fragPos;
uniform mat4 model; uniform mat4 view; uniform mat4 projection;
void main()
{
  vec4 worldPos = model * vec4(aPos, 1.0);
  fragPos = vec3(worldPos);
  gl_Position = projection * view * worldPos;
  vertexColor = aColor; texCoord = atexCoord; normal = normalize(aNormal);
})";
const char* fragmentShaderSource = R"(
#version 330 core
in vec3 vertexColor; in vec2 texCoord; in vec3 normal; in vec3 fragPos;
out vec4 FragColor;
uniform sampler2D mytexture; uniform float ambientIntensity; uniform vec3 lightPos;
void main()
{
  vec3 lightColor = vec3(1.0, 1.0, 1.0);
  vec3 lightDirection = normalize(lightPos - fragPos);
  float diff = max(dot(normal, lightDirection), 0.0);
  vec3 diffLight = diff * lightColor;
  vec3 ambientLight = ambientIntensity * lightColor;
  vec3 finalLight = ambientLight + diffLight;
  FragColor = vec4(finalLight, 1.0) * texture(mytexture, texCoord);
})";
unsigned int compileShader(unsigned int type, const char* source) {
    unsigned int shader_id = glCreateShader(type);
    glShaderSource(shader_id, 1, &source, NULL);
    glCompileShader(shader_id);
    int success; char infoLog[512]; glGetShaderiv(shader_id, GL_COMPILE_STATUS, &success);
    if (!success) { glGetShaderInfoLog(shader_id, 512, NULL, infoLog); std::cout << "ERROR::SHADER::COMPILATION_FAILED\n" << infoLog << std::endl; }
    return shader_id;
}
unsigned int createShaderProgram() {
    unsigned int vertexShader_id = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    unsigned int fragmentShader_id = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader_id);
    glAttachShader(shaderProgram, fragmentShader_id);
    glLinkProgram(shaderProgram);
    glDeleteShader(vertexShader_id);
    glDeleteShader(fragmentShader_id);
    return shaderProgram;
}


int main(void) {
    json config;
    try {
        std::ifstream f("config.json");
        config = json::parse(f);
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to load or parse config.json: " << e.what() << std::endl;
        system("pause");
        return -1;
    }

    if (!glfwInit()) {
        cout << "Failed to initialize GLFW" << endl;
        system("pause");
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    int windowWidth = config["window"]["width"];
    int windowHeight = config["window"]["height"];
    std::string windowTitle = config["window"]["title"];
    GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, windowTitle.c_str(), NULL, NULL);
    if (!window) {
        glfwTerminate();
        system("pause");
        return -1;
    }
    glfwMakeContextCurrent(window);

    if (glewInit() != GLEW_OK) {
        cout << "Failed to initialize GLEW" << endl;
        system("pause");
        return -1;
    }

    XQ::SoundManager soundManager;
    if (soundManager.init() < 0) {
        std::cout << "sound init is unsuccessful" << std::endl;
        system("pause");
        return -1;
    }
    soundManager.playMusic(config["assets"]["background_music"]);

    glViewport(0, 0, windowWidth, windowHeight);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    ResourceManager::loadTexture(config["assets"]["dog_texture"].get<std::string>(), "dog");
    ResourceManager::loadTexture(config["assets"]["cookie_texture"].get<std::string>(), "cookie");
    ResourceManager::loadTexture(config["assets"]["bg_texture"].get<std::string>(), "background");
    string eatSoundPath = config["assets"]["eat_sound"];

    float vertices[] = {
         0.5f,  0.5f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,   0.0f, 0.0f, 1.0f,
         0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,   0.0f, 0.0f, 1.0f,
        -0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f,   0.0f, 0.0f, 1.0f,
        -0.5f,  0.5f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f,   0.0f, 0.0f, 1.0f
    };
    unsigned int indices[] = { 0, 1, 3, 1, 2, 3 };
    unsigned int VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)0); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(3 * sizeof(float))); glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(6 * sizeof(float))); glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(8 * sizeof(float))); glEnableVertexAttribArray(3);
    glBindVertexArray(0);

    unsigned int shaderProgram = createShaderProgram();

    TextureInfo dogTex = ResourceManager::getTexture("dog");
    GameObject dog = { glm::vec3(0.0f), 1.5f, dogTex.id, true, dogTex.aspectRatio };

    TextureInfo cookieTex = ResourceManager::getTexture("cookie");
    vector<GameObject> cookies;
    cookies.push_back({ glm::vec3(2.5f, 2.0f, 0.0f), 0.5f, cookieTex.id, true, cookieTex.aspectRatio });
    cookies.push_back({ glm::vec3(-2.0f, -1.5f, 0.0f), 0.5f, cookieTex.id, true, cookieTex.aspectRatio });
    cookies.push_back({ glm::vec3(1.5f, -2.5f, 0.0f), 0.5f, cookieTex.id, true, cookieTex.aspectRatio });

    TextureInfo bgTex = ResourceManager::getTexture("background");
    GameObject background = { glm::vec3(0.0f, 0.0f, -5.0f), 10.0f, bgTex.id, true, bgTex.aspectRatio };

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init();

    glm::vec3 cam_position(0.0f, 0.0f, 8.0f);
    glm::vec3 cam_center(0.0f, 0.0f, 0.0f);
    float ambientIntensity = 0.8f;
    glm::vec3 lightPos(0.0f, 0.0f, 10.0f);

    float deltaTime = 0.0f;
    float lastFrame = 0.0f;
    float moveSpeed = 4.0f;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGui::Begin("Inspector");
        ImGui::Text("Dog Control");
        ImGui::DragFloat3("Position", &dog.position.x, 0.1f);
        ImGui::SliderFloat("Size", &dog.size, 0.5f, 5.0f);
        ImGui::Separator();
        ImGui::Text("Scene Control");
        ImGui::DragFloat3("Cam Position", &cam_position.x, 0.1f);
        ImGui::SliderFloat("Ambient", &ambientIntensity, 0.1f, 1.0f);
        ImGui::End();

        ImGuiIO& io = ImGui::GetIO();
        if (!io.WantCaptureKeyboard) {
            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) dog.position.y += moveSpeed * deltaTime;
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) dog.position.y -= moveSpeed * deltaTime;
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) dog.position.x -= moveSpeed * deltaTime;
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) dog.position.x += moveSpeed * deltaTime;
        }

        for (auto& cookie : cookies) {
            if (cookie.isVisible && checkCollision(dog, cookie)) {
                cookie.isVisible = false;
                soundManager.playSoundEffect(eatSoundPath);
            }
        }

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);

        glm::mat4 view = glm::lookAt(cam_position, cam_center, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)windowWidth / (float)windowHeight, 0.1f, 100.0f);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniform1f(glGetUniformLocation(shaderProgram, "ambientIntensity"), ambientIntensity);
        glUniform3fv(glGetUniformLocation(shaderProgram, "lightPos"), 1, glm::value_ptr(lightPos));
        glUniform1i(glGetUniformLocation(shaderProgram, "mytexture"), 0);

        auto renderObject = [&](const GameObject& obj) {
            if (obj.isVisible) {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, obj.textureID);
                glm::mat4 model = glm::translate(glm::mat4(1.0f), obj.position);
                model = glm::scale(model, glm::vec3(obj.size * obj.aspectRatio, obj.size, 1.0f));
                glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            }
            };

        renderObject(background);
        renderObject(dog);
        for (const auto& cookie : cookies) {
            renderObject(cookie);
        }

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    ResourceManager::clear();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);
    glfwTerminate();

    return 0;
}