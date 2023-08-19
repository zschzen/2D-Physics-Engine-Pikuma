#include "Application.h"

#include "./Physics/Constants.h"
#include "./Physics/Force.h"

bool Application::IsRunning() {
    return running;
}

///////////////////////////////////////////////////////////////////////////////
// Setup function (executed once in the beginning of the simulation)
///////////////////////////////////////////////////////////////////////////////
void Application::Setup() {
    running = Graphics::OpenWindow();

    Body* boxBody = new Body(BoxShape(200, 100), Graphics::Width() / 2.0f, Graphics::Height() / 2.0f, 1.0f);
    bodies.push_back(boxBody);
}

///////////////////////////////////////////////////////////////////////////////
// Input processing
///////////////////////////////////////////////////////////////////////////////
void Application::Input() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                running = false;
                break;
            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_ESCAPE)
                    running = false;
                if (event.key.keysym.sym == SDLK_UP)
                    pushForce.y = -50 * PIXELS_PER_METER;
                if (event.key.keysym.sym == SDLK_RIGHT)
                    pushForce.x = 50 * PIXELS_PER_METER;
                if (event.key.keysym.sym == SDLK_DOWN)
                    pushForce.y = 50 * PIXELS_PER_METER;
                if (event.key.keysym.sym == SDLK_LEFT)
                    pushForce.x = -50 * PIXELS_PER_METER;
                break;
            case SDL_KEYUP:
                if (event.key.keysym.sym == SDLK_UP)
                    pushForce.y = 0;
                if (event.key.keysym.sym == SDLK_RIGHT)
                    pushForce.x = 0;
                if (event.key.keysym.sym == SDLK_DOWN)
                    pushForce.y = 0;
                if (event.key.keysym.sym == SDLK_LEFT)
                    pushForce.x = 0;
                break;
            case SDL_MOUSEMOTION:
                mouseCursor.x = event.motion.x;
                mouseCursor.y = event.motion.y;
                break;
            case SDL_MOUSEBUTTONDOWN:
                if (!selectedBody && event.button.button == SDL_BUTTON_LEFT) {
                    int x, y;
                    SDL_GetMouseState(&x, &y);

                    // Get nearest body to the mouse cursor
                    selectedBody = FindClosestBody(Vec2(x, y));

                    mouseCursor.x = x;
                    mouseCursor.y = y;
                }
                break;
            case SDL_MOUSEBUTTONUP:
                if (selectedBody && event.button.button == SDL_BUTTON_LEFT) {
                    Vec2 impulseDirection = (selectedBody->position - mouseCursor).UnitVector();
                    float impulseMagnitude = (selectedBody->position - mouseCursor).Magnitude() * 5.0;

                    selectedBody->velocity = impulseDirection * impulseMagnitude;
                    selectedBody = nullptr;
                }
                break;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// Update function (called several times per second to update objects)
///////////////////////////////////////////////////////////////////////////////
void Application::Update() {
    // Wait some time until the reach the target frame time in milliseconds
    static int timePreviousFrame;
    int timeToWait = MILLISECS_PER_FRAME - (SDL_GetTicks() - timePreviousFrame);
    if (timeToWait > 0) {
#if __EMSCRIPTEN__
        emscripten_sleep(timeToWait);
#else
        SDL_Delay(timeToWait);
#endif
    }

    // Calculate the deltatime in seconds
    float deltaTime = (SDL_GetTicks() - timePreviousFrame) / 1000.0f;
    if (deltaTime > MILLISECS_PER_FRAME) deltaTime = MILLISECS_PER_FRAME;

    // Set the time of the current frame to be used in the next one
    timePreviousFrame = SDL_GetTicks();

    // Add forces to the bodies
    for (const auto &body: bodies) {
        // Torque
        float torque = 200.0f;
        body->AddTorque(torque);
    }

    // Integrate the acceleration and velocity to estimate the new position
    for (auto body: bodies) {
        body->Update(deltaTime);
    }

    // Keep bodys inside the screen
    for (const auto &body: bodies) {
        if (body->shape->GetType() == ShapeType::CIRCLE) {
            CircleShape *circle = dynamic_cast<CircleShape *>(body->shape);

            // Left
            if (body->position.x - circle->radius < 0) {
                body->position.x = circle->radius;
                body->velocity.x *= -0.9;
            }

            // Right
            if (body->position.x + circle->radius > Graphics::windowWidth) {
                body->position.x = Graphics::windowWidth - circle->radius;
                body->velocity.x *= -0.9;
            }

            // Top
            if (body->position.y - circle->radius < 0) {
                body->position.y = circle->radius;
                body->velocity.y *= -0.9;
            }

            // Bottom
            if (body->position.y + circle->radius > Graphics::windowHeight) {
                body->position.y = Graphics::windowHeight - circle->radius;
                body->velocity.y *= -0.9;
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// Render function (called several times per second to draw objects)
///////////////////////////////////////////////////////////////////////////////
void Application::Render() {
    Graphics::ClearScreen(0xFF0F0721);

    if (selectedBody) {
        Graphics::DrawLine(selectedBody->position.x, selectedBody->position.y, mouseCursor.x, mouseCursor.y, 0xFF0000FF);
    }
    
    for (const auto &body: bodies) {
        switch (body->shape->GetType()) {
            case ShapeType::CIRCLE: {
                CircleShape *circle = dynamic_cast<CircleShape *>(body->shape);
                Graphics::DrawCircle(body->position.x, body->position.y, circle->radius, body->rotation , body->color);
                break;
            }
            case ShapeType::BOX: {
                BoxShape *box = dynamic_cast<BoxShape *>(body->shape);
                Graphics::DrawPolygon(body->position.x, body->position.y, box->worldVertices, 0xFFFFFFFF);
                break;
            }
            default:
                break;
        }
    }
    Graphics::RenderFrame();
}

///////////////////////////////////////////////////////////////////////////////
// Destroy function to delete objects and close the window
///////////////////////////////////////////////////////////////////////////////
void Application::Destroy() {
    for (const auto &body: bodies) {
        delete body;
    }

    Graphics::CloseWindow();
}

Body *Application::FindClosestBody(const Vec2 &position)
{
    Body *closestbody = nullptr;
    float closestDistance = 0.0f;

    for (const auto &body: bodies) {
        float distance = (body->position - position).Magnitude();
        if (closestbody && distance >= closestDistance) continue;

        closestbody = body;
        closestDistance = distance;
    }

    return closestbody;
}