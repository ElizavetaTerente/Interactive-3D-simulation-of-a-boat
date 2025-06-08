#include <cstdlib>
#include <iostream>

#include "mygl/shader.h"
#include "mygl/model.h"
#include "mygl/camera.h"
#include "mygl/cube_map.h"
#include "mygl/geometry.h"
#include "mygl/framebuffer.h"

#include "boat.h"
#include "light.h"
#include "water.h"

struct Query
{
    unsigned int values[2];
    unsigned int backBuffer = 0;
    unsigned int frontBuffer = 1;
};
struct
{
    Camera camera;
    bool cameraFollowBoat;
    float zoomSpeedMultiplier;

    WaterSim waterSim;
    Model modelWater;

    Boat boat;

    CubeMap skybox;

    bool renderBlinnPhong;
    bool isDay;
    Light_Directional lightSun;
    Light_Spot lightSpots[4];

    ShaderProgram shaderColor;
    ShaderProgram shaderWaterColor;
    ShaderProgram shaderWater;
    ShaderProgram shaderBlinnPhong;
    ShaderProgram shaderSkybox;

    Framebuffer customFramebuffer;
    bool useBinarySearch;

    Query query;

} sScene;

struct
{
    bool mouseButtonPressed = false;
    Vector2D mousePressStart;
    bool keyPressed[Boat::eControl::CONTROL_COUNT] = {false, false, false, false};
} sInput;

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    /* input for camera control */
    if(key == GLFW_KEY_0 && action == GLFW_PRESS)
    {
        sScene.cameraFollowBoat = false;
        sScene.camera.lookAt = {0.0f, 0.0f, 0.0f};
        cameraUpdateOrbit(sScene.camera, {0.0f, 0.0f}, 0.0f);
    }
    if(key == GLFW_KEY_1 && action == GLFW_PRESS)
    {
        sScene.cameraFollowBoat = false;
    }
    if(key == GLFW_KEY_2 && action == GLFW_PRESS)
    {
        sScene.cameraFollowBoat = true;
    }

    if(key == GLFW_KEY_C && action == GLFW_PRESS)
    {
        sScene.renderBlinnPhong = !sScene.renderBlinnPhong;
    }

    if(key == GLFW_KEY_B && action == GLFW_PRESS)
    {
        sScene.useBinarySearch = !sScene.useBinarySearch;
    }

    /* night light setting */
    if(key == GLFW_KEY_N && action == GLFW_PRESS)
    {
        sScene.lightSun.ambient = { 0.0, 0.0, 0.1 };
        sScene.lightSun.color = { 0.1, 0.1, 0.2 };
        sScene.isDay = false;
    }

    /* day light setting */
    if(key == GLFW_KEY_M && action == GLFW_PRESS)
    {
        sScene.lightSun.ambient = { 0.2, 0.2, 0.2 };
        sScene.lightSun.color = { 0.7, 0.7, 0.7 };
        sScene.isDay = true;
    }

    /* toggle boat lights */
    if(key == GLFW_KEY_L && action == GLFW_PRESS)
    {
        sScene.lightSpots[0].enabled = !sScene.lightSpots[0].enabled;
        sScene.lightSpots[1].enabled = !sScene.lightSpots[1].enabled;
        sScene.lightSpots[2].enabled = !sScene.lightSpots[2].enabled;
        sScene.lightSpots[3].enabled = !sScene.lightSpots[3].enabled;
    }

    /* input for boat control */
    if(key == GLFW_KEY_W)
    {
        sInput.keyPressed[Boat::eControl::THROTTLE_UP] = (action == GLFW_PRESS || action == GLFW_REPEAT);
    }
    if(key == GLFW_KEY_S)
    {
        sInput.keyPressed[Boat::eControl::THROTTLE_DOWN] = (action == GLFW_PRESS || action == GLFW_REPEAT);
    }

    if(key == GLFW_KEY_A)
    {
        sInput.keyPressed[Boat::eControl::RUDDER_LEFT] = (action == GLFW_PRESS || action == GLFW_REPEAT);
    }
    if(key == GLFW_KEY_D)
    {
        sInput.keyPressed[Boat::eControl::RUDDER_RIGHT] = (action == GLFW_PRESS || action == GLFW_REPEAT);
    }

    /* close window on escape */
    if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, true);
    }

    /* make screenshot and save in work directory */
    if(key == GLFW_KEY_P && action == GLFW_PRESS)
    {
        screenshotToPNG("screenshot.png");
    }
}

void mousePosCallback(GLFWwindow* window, double x, double y)
{
    if(sInput.mouseButtonPressed)
    {
        Vector2D diff = sInput.mousePressStart - Vector2D(x, y);
        cameraUpdateOrbit(sScene.camera, diff, 0.0f);
        sInput.mousePressStart = Vector2D(x, y);
    }
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if(button == GLFW_MOUSE_BUTTON_LEFT)
    {
        sInput.mouseButtonPressed = (action == GLFW_PRESS);

        double x, y;
        glfwGetCursorPos(window, &x, &y);
        sInput.mousePressStart = Vector2D(x, y);
    }
}

void mouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    cameraUpdateOrbit(sScene.camera, {0, 0}, sScene.zoomSpeedMultiplier * yoffset);
}


void windowResizeCallback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
    sScene.camera.width = width;
    sScene.camera.height = height;
//    delete framebuffer and create a new one with adjusted dimension
    deleteFramebuffer(sScene.customFramebuffer);
    sScene.customFramebuffer = createFramebuffer(width, height);
}

void sceneInit(float width, float height)
{
    sScene.camera = cameraCreate(width, height, to_radians(45.0), 0.01, 500.0, {10.0, 10.0, 10.0}, {0.0, 0.0, 0.0});
    sScene.cameraFollowBoat = true;
    sScene.zoomSpeedMultiplier = 0.05f;

    sScene.boat = boatLoad("assets/boat/boat.obj");
    sScene.modelWater = modelLoad("assets/water_01/water.obj").front();

    sScene.renderBlinnPhong = true;

    Vector3D spotLight = normalize(Vector3D(0.0, -0.3, 1.0));
    Vector3D navLights = normalize(Vector3D(1.0, 0.0, 0.0));

    sScene.lightSun = { .direction = {-1.0, -0.1, -1.0}, .ambient = { 0.0, 0.0, 0.1 }, .color =  { 0.1, 0.1, 0.2 } };
    sScene.lightSpots[0] = { .position = { 0.1, 1.63, -2.1}, .direction =-navLights, .color = {1.0, 0.0, 0.0}, .constant = 1.0, .linear = 0.045, .quadratic = 0.027, .cutoff = to_radians(180.0f) };
    sScene.lightSpots[1] = { .position = {-0.1, 1.63, -2.1}, .direction = navLights, .color = {0.0, 1.0, 0.0}, .constant = 1.0, .linear = 0.045, .quadratic = 0.027, .cutoff = to_radians(180.0f) };
    sScene.lightSpots[2] = { .position = { 0.3, 1.63,  1.43}, .direction = spotLight, .color = {1.0, 1.0, 1.0}, .constant = 1.0, .linear = 0.14, .quadratic = 0.07, .cutoff = to_radians(75.0f) };
    sScene.lightSpots[3] = { .position = {-0.3, 1.63,  1.43}, .direction = spotLight, .color = {1.0, 1.0, 1.0}, .constant = 1.0, .linear = 0.14, .quadratic = 0.07, .cutoff = to_radians(75.0f) };

    sScene.skybox = cubeMapCreate(cube::vertexPos, cube::indices, {"assets/kloofendal_48d_partly_cloudy/px.png", "assets/kloofendal_48d_partly_cloudy/nx.png", "assets/kloofendal_48d_partly_cloudy/py.png", "assets/kloofendal_48d_partly_cloudy/ny.png", "assets/kloofendal_48d_partly_cloudy/pz.png", "assets/kloofendal_48d_partly_cloudy/nz.png"});

    sScene.shaderColor = shaderLoad("shader/default.vert", "shader/color.frag");
    sScene.shaderWaterColor = shaderLoad("shader/water.vert", "shader/color.frag");
    sScene.shaderWater = shaderLoad("shader/water.vert", "shader/blinn_phong_water.frag");
    sScene.shaderBlinnPhong = shaderLoad("shader/default.vert", "shader/blinn_phong.frag");
    sScene.shaderSkybox = shaderLoad("shader/skybox.vert", "shader/skybox.frag");

    sScene.customFramebuffer = createFramebuffer(width, height);
    sScene.useBinarySearch = true;

//    for getting gpu time
    glGenQueries(1, &sScene.query.values[sScene.query.backBuffer]);
    glGenQueries(1, &sScene.query.values[sScene.query.frontBuffer]);
//    dummy query for first access
    glBeginQuery(GL_TIME_ELAPSED, sScene.query.values[sScene.query.frontBuffer]);
    glEndQuery(GL_TIME_ELAPSED);
}

void sceneUpdate(float dt)
{
    sScene.waterSim.accumTime += dt;
    boatMove(sScene.boat, sScene.waterSim, sInput.keyPressed, dt);

    if (sScene.cameraFollowBoat)
        cameraFollow(sScene.camera, sScene.boat.position);
}

void swapQueryBuffers() {
    if (sScene.query.backBuffer) {
        sScene.query.backBuffer = 0;
        sScene.query.frontBuffer = 1;
    } else {
        sScene.query.backBuffer = 1;
        sScene.query.frontBuffer = 0;
    }
}

void renderBoat() {
    Matrix4D proj = cameraProjection(sScene.camera);
    Matrix4D view = cameraView(sScene.camera);

    glUseProgram(sScene.shaderBlinnPhong.id);
    shaderUniform(sScene.shaderBlinnPhong, "uProj",  proj);
    shaderUniform(sScene.shaderBlinnPhong, "uView",  view);
    shaderUniform(sScene.shaderBlinnPhong, "uModel",  sScene.boat.transformation);
    shaderUniform(sScene.shaderBlinnPhong, "uViewPos", sScene.camera.position);

    /* set directional light source */
    shaderUniform(sScene.shaderBlinnPhong, "uLightSun.direction", sScene.lightSun.direction);
    shaderUniform(sScene.shaderBlinnPhong, "uLightSun.ambient", sScene.lightSun.ambient);
    shaderUniform(sScene.shaderBlinnPhong, "uLightSun.color", sScene.lightSun.color);

    /* set boat's spotlights */
    for(int i = 0; i < 4; i++)
    {
        Vector4D pos = sScene.boat.transformation * Vector4D(sScene.lightSpots[i].position);
        Vector4D dir = Matrix4D(Matrix3D(sScene.boat.transformation)) * Vector4D(sScene.lightSpots[i].direction);

        std::string light = "uLightSpots[" + std::to_string(i) + "]";
        shaderUniform(sScene.shaderBlinnPhong, light + ".position", Vector3D{pos.x, pos.y, pos.z});
        shaderUniform(sScene.shaderBlinnPhong, light + ".direction", Vector3D{dir.x, dir.y, dir.z});
        shaderUniform(sScene.shaderBlinnPhong, light + ".color", sScene.lightSpots[i].color);
        shaderUniform(sScene.shaderBlinnPhong, light + ".constant", sScene.lightSpots[i].constant);
        shaderUniform(sScene.shaderBlinnPhong, light + ".linear", sScene.lightSpots[i].linear);
        shaderUniform(sScene.shaderBlinnPhong, light + ".quadratic", sScene.lightSpots[i].quadratic);
        shaderUniform(sScene.shaderBlinnPhong, light + ".cutoff", sScene.lightSpots[i].cutoff);
        shaderUniform(sScene.shaderBlinnPhong, light + ".enabled", sScene.lightSpots[i].enabled);
    }

    for(unsigned int i = 0; i < sScene.boat.partModel.size(); i++)
    {
        auto& model = sScene.boat.partModel[i];
        glBindVertexArray(model.mesh.vao);

        shaderUniform(sScene.shaderBlinnPhong, "uModel", sScene.boat.transformation);

        for(auto& material : model.material){

            shaderUniform(sScene.shaderBlinnPhong, "uMaterial.shininess", material.shininess);

//            Bind Textures and pass them to the shader
//            Because we draw the elements in each iteration, we can overwrite the GL_TEXTURE
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, material.map_diffuse.id);
            shaderUniform(sScene.shaderBlinnPhong, "uMaterial.diffuse", 0);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, material.map_specular.id);
            shaderUniform(sScene.shaderBlinnPhong, "uMaterial.specular", 1);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, material.map_normal.id);
            shaderUniform(sScene.shaderBlinnPhong, "uMaterial.normal", 2);
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, material.map_ambient.id);
            shaderUniform(sScene.shaderBlinnPhong, "uMaterial.ambient", 3);

            /*---- reflection ----*/
            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_CUBE_MAP, sScene.skybox.texture.id);
            shaderUniform(sScene.shaderBlinnPhong, "uSkybox", 4);

            glDrawElements(GL_TRIANGLES, material.indexCount, GL_UNSIGNED_INT, (const void*) (material.indexOffset*sizeof(unsigned int)) );
        }
    }
}

void renderBlinnPhong()
{
//    get gpu time
    GLuint64 time;
    glBeginQuery(GL_TIME_ELAPSED, sScene.query.values[sScene.query.backBuffer]);

    /* setup camera and model matrices */
    Matrix4D proj = cameraProjection(sScene.camera);
    Matrix4D view = cameraView(sScene.camera);

    /*--------------------- render boat into custom framebuffer ---------------------*/

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, sScene.customFramebuffer.id);
    glClearColor(0., 0., 0., 1.);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    renderBoat();

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

    /*--------------------- render water ---------------------*/

    glUseProgram(sScene.shaderWater.id);

    shaderUniform(sScene.shaderWater, "uProj",  proj);
    shaderUniform(sScene.shaderWater, "uView",  view);
    shaderUniform(sScene.shaderWater, "uViewPos", sScene.camera.position);
    shaderUniform(sScene.shaderWater, "uModel", Matrix4D::identity());

    /* set directional light source */
    shaderUniform(sScene.shaderWater, "uLightSun.direction", sScene.lightSun.direction);
    shaderUniform(sScene.shaderWater, "uLightSun.ambient", sScene.lightSun.ambient);
    shaderUniform(sScene.shaderWater, "uLightSun.color", sScene.lightSun.color);

    /* set boat's spotlights */
    for(int i = 0; i < 4; i++)
    {
        Vector4D pos = sScene.boat.transformation * Vector4D(sScene.lightSpots[i].position);
        auto dir = Matrix3D(sScene.boat.transformation) * sScene.lightSpots[i].direction;

        std::string light = "uLightSpots[" + std::to_string(i) + "]";
        shaderUniform(sScene.shaderWater, light + ".position", Vector3D{pos.x, pos.y, pos.z});
        shaderUniform(sScene.shaderWater, light + ".direction", Vector3D{dir.x, dir.y, dir.z});
        shaderUniform(sScene.shaderWater, light + ".color", sScene.lightSpots[i].color);
        shaderUniform(sScene.shaderWater, light + ".constant", sScene.lightSpots[i].constant);
        shaderUniform(sScene.shaderWater, light + ".linear", sScene.lightSpots[i].linear);
        shaderUniform(sScene.shaderWater, light + ".quadratic", sScene.lightSpots[i].quadratic);
        shaderUniform(sScene.shaderWater, light + ".cutoff", sScene.lightSpots[i].cutoff);
        shaderUniform(sScene.shaderWater, light + ".enabled", sScene.lightSpots[i].enabled);
    }

    /* set wave params */
    shaderUniform(sScene.shaderWater, "time", sScene.waterSim.accumTime);
    for(int i = 0; i < 3; i++)
    {
        std::string wave = "water_sim[" + std::to_string(i) + "]";
        shaderUniform(sScene.shaderWater, wave + ".amplitude", sScene.waterSim.parameter[i].amplitude);
        shaderUniform(sScene.shaderWater, wave + ".phi", sScene.waterSim.parameter[i].phi);
        shaderUniform(sScene.shaderWater, wave + ".omega", sScene.waterSim.parameter[i].omega);
        shaderUniform(sScene.shaderWater, wave + ".direction", sScene.waterSim.parameter[i].direction);
    }

    glBindVertexArray(sScene.modelWater.mesh.vao);

    for(auto& material : sScene.modelWater.material)
    {

        shaderUniform(sScene.shaderWater, "uMaterial.shininess", material.shininess);

        // Set uniforms and bind water textures
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, material.map_diffuse.id);
        shaderUniform(sScene.shaderWater, "uMaterial.diffuse", 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, material.map_specular.id);
        shaderUniform(sScene.shaderWater, "uMaterial.specular", 1);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, material.map_normal.id);
        shaderUniform(sScene.shaderWater, "uMaterial.normal", 2);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, material.map_ambient.id);
        shaderUniform(sScene.shaderWater, "uMaterial.ambient", 3);

        /*---- environment mapping ----*/
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_CUBE_MAP, sScene.skybox.texture.id);
        shaderUniform(sScene.shaderWater, "uSkybox", 4);

        /*-- Screen Space Reflection --*/

        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, sScene.customFramebuffer.colorTexture);
        shaderUniform(sScene.shaderWater, "uBoatColor", 5);
        glActiveTexture(GL_TEXTURE6);
        glBindTexture(GL_TEXTURE_2D, sScene.customFramebuffer.depthTexture);
        shaderUniform(sScene.shaderWater, "uBoatDepth", 6);
        shaderUniform(sScene.shaderWater, "uUseBinarySearch", sScene.useBinarySearch);

        glDrawElements(GL_TRIANGLES, material.indexCount, GL_UNSIGNED_INT, (const void*) (material.indexOffset*sizeof(unsigned int)) );
    }

    /*--------- render boat into default framebuffer --------*/

    renderBoat();

    /*-------------------- render skybox --------------------*/
//    see: https://learnopengl.com/Advanced-OpenGL/Cubemaps but with our mesh with vertices and indices
    glDepthFunc(GL_LEQUAL);
    glUseProgram(sScene.shaderSkybox.id);
//    remove translation from view matrix
    shaderUniform(sScene.shaderSkybox, "uView", Matrix4D(Matrix3D(view)));
    shaderUniform(sScene.shaderSkybox, "uProj", proj);
    shaderUniform(sScene.shaderSkybox, "uDirectionalLightColor", sScene.lightSun.color);
    glBindVertexArray(sScene.skybox.mesh.vao);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, sScene.skybox.texture.id);
    shaderUniform(sScene.shaderSkybox, "uSkybox", 0);
    glDrawElements(GL_TRIANGLES, sScene.skybox.mesh.size_ibo, GL_UNSIGNED_INT, nullptr );
    glDepthFunc(GL_LESS);

    glEndQuery(GL_TIME_ELAPSED);
    glGetQueryObjectui64v(sScene.query.values[sScene.query.frontBuffer], GL_QUERY_RESULT, &time);
    printf("GPU time elapsed for one frame: %lf ms\n", time / 1000000.0);
    swapQueryBuffers();

    /* cleanup opengl state */
    glBindVertexArray(0);
    glUseProgram(0);
}

void renderColor()
{
    /* setup camera and model matrices */
    Matrix4D proj = cameraProjection(sScene.camera);
    Matrix4D view = cameraView(sScene.camera);

    glUseProgram(sScene.shaderColor.id);
    shaderUniform(sScene.shaderColor, "uProj",  proj);
    shaderUniform(sScene.shaderColor, "uView",  view);
    shaderUniform(sScene.shaderColor, "uModel",  sScene.boat.transformation);

    /* render boat */
    for(unsigned int i = 0; i < sScene.boat.partModel.size(); i++)
    {
        auto& model = sScene.boat.partModel[i];
        glBindVertexArray(model.mesh.vao);

        shaderUniform(sScene.shaderColor, "uModel", sScene.boat.transformation);

        for(auto& material : model.material)
        {
            /* set material properties */
            shaderUniform(sScene.shaderColor, "uMaterial.diffuse", material.diffuse);

            glDrawElements(GL_TRIANGLES, material.indexCount, GL_UNSIGNED_INT, (const void*) (material.indexOffset*sizeof(unsigned int)) );
        }
    }

    /* render water */
    glUseProgram(sScene.shaderWaterColor.id);

    shaderUniform(sScene.shaderWaterColor, "uProj",  proj);
    shaderUniform(sScene.shaderWaterColor, "uView",  view);
    shaderUniform(sScene.shaderWaterColor, "uModel", Matrix4D::identity());

    /* set wave params */
    shaderUniform(sScene.shaderWaterColor, "time", sScene.waterSim.accumTime);
    for(int i = 0; i < 3; i++)
    {
        std::string wave = "water_sim[" + std::to_string(i) + "]";
        shaderUniform(sScene.shaderWaterColor, wave + ".amplitude", sScene.waterSim.parameter[i].amplitude);
        shaderUniform(sScene.shaderWaterColor, wave + ".phi", sScene.waterSim.parameter[i].phi);
        shaderUniform(sScene.shaderWaterColor, wave + ".omega", sScene.waterSim.parameter[i].omega);
        shaderUniform(sScene.shaderWaterColor, wave + ".direction", sScene.waterSim.parameter[i].direction);
    }

    glBindVertexArray(sScene.modelWater.mesh.vao);

    for(auto& material : sScene.modelWater.material)
    {
        shaderUniform(sScene.shaderWaterColor, "uMaterial.diffuse", material.diffuse);

        glDrawElements(GL_TRIANGLES, material.indexCount, GL_UNSIGNED_INT, (const void*) (material.indexOffset*sizeof(unsigned int)) );
    }


    /* cleanup opengl state */
    glBindVertexArray(0);
    glUseProgram(0);
}

void sceneDraw()
{
    if (sScene.isDay)
    {
        glClearColor(135.0 / 255, 206.0 / 255, 235.0 / 255, 1.0);
    }
    else
    {
        glClearColor(0.2, 0.2, 0.3, 1.0);
    }
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    /*------------ render scene -------------*/
    {
        if (sScene.renderBlinnPhong)
        {
            renderBlinnPhong();
        }
        else
        {
            renderColor();
        }
    }
}

int main(int argc, char** argv)
{
    /*---------- init window ------------*/
    int width = 1280;
    int height = 720;
    GLFWwindow* window = windowCreate("Project - Screen Space Reflection", width, height);
    if(!window) { return EXIT_FAILURE; }

    /* set window callbacks */
    glfwSetKeyCallback(window, keyCallback);
    glfwSetCursorPosCallback(window, mousePosCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetScrollCallback(window, mouseScrollCallback);
    glfwSetFramebufferSizeCallback(window, windowResizeCallback);

    /*---------- init opengl stuff ------------*/
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);

    /* setup scene */
    sceneInit(width, height);

    /*-------------- main loop ----------------*/
    double timeStamp = glfwGetTime();
    double timeStampNew = 0.0;
    while(!glfwWindowShouldClose(window))
    {
        /* poll and process input and window events */
        glfwPollEvents();

        /* update scene */
        timeStampNew = glfwGetTime();
        sceneUpdate(timeStampNew - timeStamp);
        timeStamp = timeStampNew;

        /* draw all objects in the scene */
        sceneDraw();
        
        /* swap front and back buffer */
        glfwSwapBuffers(window);
    }

    /*-------- cleanup --------*/
    shaderDelete(sScene.shaderWater);
    shaderDelete(sScene.shaderBlinnPhong);
    shaderDelete(sScene.shaderWaterColor);
    shaderDelete(sScene.shaderColor);
    shaderDelete(sScene.shaderSkybox);
    windowDelete(window);

    return EXIT_SUCCESS;
}
