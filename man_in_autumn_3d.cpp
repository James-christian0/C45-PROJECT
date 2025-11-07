#include <iostream>
#include <cmath>
#include <vector>
#include <cstdlib>
#include <ctime>

#include <GL/gl.h>
#include <GL/glu.h>

#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

using namespace std;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// --- Global Constants ---
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
const int NUM_LEAVES = 150; 

// --- Camera & Interaction Globals ---
float manPositionX = 0.0f;
float manPositionZ = 0.0f; // New: Z-axis position for W/S movement
float cameraAngle = 45.0f;
float distanceFromMan = 250.0f; // Initial zoom distance
const float MIN_ZOOM = 50.0f;
const float MAX_ZOOM = 500.0f;

// Mouse state for rotation and zoom
int lastMouseX = 0;
int lastMouseY = 0;
bool isRotating = false;
bool isZooming = false;

// --- Animation Globals ---
float leafDriftSpeed = 0.0f;
float jacketColor[3] = {0.8f, 0.2f, 0.0f};
float walkPhase = 0.0f; // Controls the man's gait cycle for walking animation
bool isManMoving = false; // Tracks if the man is actively moving

// --- Leaf Structure (3D) ---
struct Leaf {
    float x, y, z;
    float color[3];
    float size;
    float fallSpeed;
};
vector<Leaf> fallingLeaves;

// --- Utility Functions ---

void setMaterialColor(float r, float g, float b) {
    GLfloat ambient[] = { r * 0.2f, g * 0.2f, b * 0.2f, 1.0f };
    GLfloat diffuse[] = { r, g, b, 1.0f };
    glMaterialfv(GL_FRONT, GL_AMBIENT, ambient);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuse);
    glColor3f(r, g, b);
}

/**
 * Draws a solid cylinder using GLU primitives, height is along the Z-axis.
 */
void drawCylinder(float baseRadius, float topRadius, float height) {
    GLUquadricObj *quadric = gluNewQuadric();
    gluQuadricNormals(quadric, GLU_SMOOTH);
    gluCylinder(quadric, baseRadius, topRadius, height, 16, 1);
    gluDeleteQuadric(quadric);
}

void initializeLeaves() {
    srand(time(0));
    fallingLeaves.clear();

    for (int i = 0; i < NUM_LEAVES; ++i) {
        Leaf l;
        l.x = (rand() % 400) - 200.0f;
        l.y = (rand() % 300) + 200.0f;
        l.z = (rand() % 400) - 200.0f;

        // Random autumn colors
        float r = static_cast <float> (rand()) / RAND_MAX;
        if (r < 0.25f) { l.color[0] = 0.8f; l.color[1] = 0.2f; l.color[2] = 0.0f; } // Red
        else if (r < 0.50f) { l.color[0] = 1.0f; l.color[1] = 0.5f; l.color[2] = 0.0f; } // Orange
        else if (r < 0.75f) { l.color[0] = 1.0f; l.color[1] = 1.0f; l.color[2] = 0.0f; } // Yellow
        else { l.color[0] = 0.5f; l.color[1] = 0.3f; l.color[2] = 0.1f; } // Brown

        l.size = 1.5f + (static_cast <float> (rand() % 100) / 100.0f) * 1.5f;
        l.fallSpeed = 0.5f + (static_cast <float> (rand() % 100) / 100.0f) * 1.5f;
        fallingLeaves.push_back(l);
    }
}

void drawGround() {
    setMaterialColor(0.25f, 0.15f, 0.07f); // Rich Dirt Brown
    glBegin(GL_QUADS);
    glNormal3f(0.0f, 1.0f, 0.0f);
    glVertex3f(-1000.0f, 0.0f, -1000.0f);
    glVertex3f(1000.0f, 0.0f, -1000.0f);
    glVertex3f(1000.0f, 0.0f, 1000.0f);
    glVertex3f(-1000.0f, 0.0f, 1000.0f);
    glEnd();
}

/**
 * Draws the enhanced 3D man with a dynamic walking animation.
 */
void draw3DMan(float x, float y, float z) {
    glPushMatrix();
    glTranslatef(x, y, z);
    glRotatef(90.0f, 0.0f, 1.0f, 0.0f); // Face the camera

    const float MAN_HEIGHT = 100.0f;
    const float TORSO_HEIGHT = MAN_HEIGHT * 0.45f;
    const float LEG_LENGTH = MAN_HEIGHT * 0.45f;
    const float ARM_LENGTH = MAN_HEIGHT * 0.40f;
    const float BODY_RADIUS = 12.0f;
    const float LIMB_RADIUS = 5.0f;

    // Head (Skin tone)
    setMaterialColor(1.0f, 0.8f, 0.7f);
    glPushMatrix();
    glTranslatef(0.0f, TORSO_HEIGHT + LEG_LENGTH + 10.0f, 0.0f);
    glutSolidSphere(10.0f, 16, 16);
    glPopMatrix();

    // Torso (Jacket)
    setMaterialColor(jacketColor[0], jacketColor[1], jacketColor[2]);
    glPushMatrix();
    glTranslatef(0.0f, LEG_LENGTH, 0.0f);
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f); // Orient cylinder vertically
    drawCylinder(BODY_RADIUS, BODY_RADIUS * 0.8f, TORSO_HEIGHT);
    glPopMatrix();

    // --- Arms (Animated Walk Cycle) ---
    float armAngle = 20.0f * sin(walkPhase);
    setMaterialColor(jacketColor[0] * 0.8f, jacketColor[1] * 0.8f, jacketColor[2] * 0.8f);

    for (int i = -1; i <= 1; i += 2) {
        glPushMatrix();
        glTranslatef(i * BODY_RADIUS, LEG_LENGTH + TORSO_HEIGHT * 0.8f, 0.0f);
        
        // Pivot point for shoulder
        glRotatef(i * armAngle, 1.0f, 0.0f, 0.0f); // Rotate based on walk phase
        
        // Draw Arm
        glTranslatef(0.0f, 0.0f, 0.0f); 
        glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
        drawCylinder(LIMB_RADIUS, LIMB_RADIUS * 0.8f, ARM_LENGTH);
        
        // Hand (small sphere)
        glPushMatrix();
        glTranslatef(0.0f, 0.0f, ARM_LENGTH);
        glutSolidSphere(LIMB_RADIUS * 0.8f, 10, 10);
        glPopMatrix();
        
        glPopMatrix();
    }

    // --- Legs (Animated Walk Cycle) ---
    float legAngle = 30.0f * sin(walkPhase);
    setMaterialColor(0.1f, 0.1f, 0.5f); // Pants/Denim

    for (int i = -1; i <= 1; i += 2) {
        glPushMatrix();
        glTranslatef(i * LIMB_RADIUS, LEG_LENGTH, 0.0f);
        
        // Pivot point for hip
        glRotatef(i * legAngle, 1.0f, 0.0f, 0.0f); // Rotate based on walk phase (opposite to the other leg)
        
        // Draw Leg
        glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
        drawCylinder(LIMB_RADIUS + 2.0f, LIMB_RADIUS, LEG_LENGTH);
        
        glPopMatrix();
    }

    glPopMatrix();
}

void draw3DTree(float x, float z) {
    glPushMatrix();
    glTranslatef(x, 0.0f, z);

    // Trunk (Brown)
    setMaterialColor(0.3f, 0.15f, 0.05f);
    glPushMatrix();
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
    drawCylinder(15.0f, 10.0f, 120.0f); // Tapered trunk
    glPopMatrix();

    // Canopy (Autumn Colors)
    glPushMatrix();
    glTranslatef(0.0f, 120.0f, 0.0f);
    
    // Bottom cone (Darker orange)
    setMaterialColor(0.8f, 0.4f, 0.0f);
    glutSolidCone(60.0f, 70.0f, 16, 16);
    
    // Top cone (Lighter orange/yellow)
    glTranslatef(0.0f, 35.0f, 0.0f);
    setMaterialColor(0.9f, 0.6f, 0.1f);
    glutSolidCone(70.0f, 70.0f, 16, 16);

    glPopMatrix();
    glPopMatrix();
}

void draw3DLeaves() {
    // Enable blending for transparent effect of flat quads
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glBegin(GL_QUADS);
    for (const auto& leaf : fallingLeaves) {
        setMaterialColor(leaf.color[0], leaf.color[1], leaf.color[2]);

        // Calculate wind offset and subtle sway for realism
        float currentX = leaf.x + 20.0f * sin(leafDriftSpeed + leaf.z * 0.1f);
        float currentY = leaf.y + 5.0f * cos(leafDriftSpeed * 2.0f + leaf.x * 0.1f);

        // Draw flat quads (simple billboard effect)
        glVertex3f(currentX, currentY, leaf.z);
        glVertex3f(currentX + leaf.size, currentY, leaf.z);
        glVertex3f(currentX + leaf.size, currentY + leaf.size, leaf.z);
        glVertex3f(currentX, currentY + leaf.size, leaf.z);
    }
    glEnd();
    
    glDisable(GL_BLEND);
}

// --- OpenGL Setup and Callbacks ---

void reshape(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, (GLfloat)w / (GLfloat)h, 1.0, 1000.0);
    glMatrixMode(GL_MODELVIEW);
}

void initialize() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glClearColor(0.8f, 0.9f, 0.95f, 1.0f); // Autumn Sky

    // Lighting Setup
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    float light_ambient[] = { 0.3f, 0.3f, 0.3f, 1.0f };
    float light_diffuse[] = { 0.7f, 0.7f, 0.7f, 1.0f };
    float light_position[] = { 200.0f, 400.0f, 100.0f, 0.0f };
    glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);

    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
    
    // Atmospheric Fog for realism
    glEnable(GL_FOG);
    glFogi(GL_FOG_MODE, GL_EXP2);
    GLfloat fogColor[4] = {0.8f, 0.9f, 0.95f, 1.0f};
    glFogfv(GL_FOG_COLOR, fogColor);
    glFogf(GL_FOG_DENSITY, 0.005f);

    glShadeModel(GL_SMOOTH);
    initializeLeaves();
}

void renderScene() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // --- Camera (gluLookAt) ---
    // Calculate the camera position orbiting the center (0, 100, 0)
    float camX = distanceFromMan * sin(cameraAngle * M_PI / 180.0f);
    float camZ = distanceFromMan * cos(cameraAngle * M_PI / 180.0f);

    // Camera is orbiting the center of the scene (0, 50, 0)
    gluLookAt(camX, 150.0f, camZ + 30.0f, 
              0.0f, 50.0f, 0.0f,           
              0.0f, 1.0f, 0.0f);            

    // Draw elements
    drawGround();
    draw3DTree(150.0f, -100.0f);
    draw3DTree(-150.0f, 50.0f);
    draw3DTree(50.0f, 200.0f); 
    draw3DMan(manPositionX, 0.0f, manPositionZ); // Man uses X and Z positions
    draw3DLeaves();

    glutSwapBuffers();
}

// --- Animation & Timer Function ---

void updateScene(int value) {
    // 1. Update leaf positions
    for (auto& leaf : fallingLeaves) {
        leaf.y -= leaf.fallSpeed;
        if (leaf.y < 0) {
            leaf.y = 500.0f + (rand() % 100);
            leaf.x = (rand() % 400) - 200.0f;
            leaf.z = (rand() % 400) - 200.0f;
            leaf.fallSpeed = 0.5f + (static_cast <float> (rand() % 100) / 100.0f) * 1.5f;
        }
    }

    // 2. Update horizontal wind/drift
    leafDriftSpeed += 0.02f;

    // 3. Update Man's Walk Cycle and Jacket Color
    if (isManMoving) {
        walkPhase = fmod(walkPhase + 0.3f, 2.0f * M_PI); // Increase walk phase if moving
    } else {
        // Slowly dampen walk phase to simulate a stop
        if (walkPhase > 0.1f) {
            walkPhase *= 0.8;
        } else {
            walkPhase = 0.0f;
        }
    }
    isManMoving = false; // Reset the flag for the next frame

    static float hue = 0.0f;
    hue = fmod(hue + 0.002f, 1.0f);
    if (hue < 0.333f) { jacketColor[0] = 1.0f; jacketColor[1] = hue * 3.0f; jacketColor[2] = 0.0f; }
    else if (hue < 0.666f) { jacketColor[0] = 1.0f - (hue - 0.333f) * 3.0f; jacketColor[1] = 1.0f; jacketColor[2] = (hue - 0.333f) * 3.0f; }
    else { jacketColor[0] = (hue - 0.666f) * 3.0f; jacketColor[1] = 1.0f - (hue - 0.666f) * 3.0f; jacketColor[2] = 1.0f; }

    glutPostRedisplay();
    glutTimerFunc(16, updateScene, 0); // ~60 FPS
}

// --- Input Handlers (WASD) ---

void keyboardInput(unsigned char key, int x, int y) {
    float move_step = 5.0f;
    
    // WASD Movement (A/D for X, W/S for Z)
    if (key == 'a' || key == 'A') { manPositionX -= move_step; isManMoving = true; } 
    else if (key == 'd' || key == 'D') { manPositionX += move_step; isManMoving = true; } 
    else if (key == 'w' || key == 'W') { manPositionZ -= move_step; isManMoving = true; } // Move forward
    else if (key == 's' || key == 'S') { manPositionZ += move_step; isManMoving = true; } // Move backward
    
    // Boundary checks (optional, keeps man within a reasonable range)
    if (manPositionX < -300.0f) manPositionX = -300.0f;
    if (manPositionX > 300.0f) manPositionX = 300.0f;
    if (manPositionZ < -300.0f) manPositionZ = -300.0f;
    if (manPositionZ > 300.0f) manPositionZ = 300.0f;

    glutPostRedisplay();
}

// --- Input Handlers (Arrow Keys for Zoom/Movement) ---

void specialKeyInput(int key, int x, int y) {
    float move_step = 5.0f;
    float zoom_step = 10.0f;

    switch (key) {
        case GLUT_KEY_LEFT: // Move Left
            manPositionX -= move_step;
            isManMoving = true;
            break;
        case GLUT_KEY_RIGHT: // Move Right
            manPositionX += move_step;
            isManMoving = true;
            break;
        case GLUT_KEY_UP: // Zoom In (Camera closer)
            distanceFromMan -= zoom_step;
            if (distanceFromMan < MIN_ZOOM) distanceFromMan = MIN_ZOOM;
            break;
        case GLUT_KEY_DOWN: // Zoom Out (Camera farther)
            distanceFromMan += zoom_step;
            if (distanceFromMan > MAX_ZOOM) distanceFromMan = MAX_ZOOM;
            break;
    }
    
    glutPostRedisplay();
}

// --- Mouse Input Handlers (Rotation and Zoom) ---

void mouseInput(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON) {
        isRotating = (state == GLUT_DOWN);
        if (state == GLUT_DOWN) {
            lastMouseX = x;
        }
    } else if (button == GLUT_RIGHT_BUTTON) {
        isZooming = (state == GLUT_DOWN);
        if (state == GLUT_DOWN) {
            lastMouseY = y;
        }
    }
    glutPostRedisplay();
}

void mouseMove(int x, int y) {
    if (isRotating) {
        // Left drag for camera rotation
        float rotationSpeed = 0.5f;
        cameraAngle -= (x - lastMouseX) * rotationSpeed;
        lastMouseX = x;
    } else if (isZooming) {
        // Right drag for camera zoom (vertical movement controls distance)
        float zoomSpeed = 2.0f;
        distanceFromMan += (y - lastMouseY) * zoomSpeed; // Drag down (y increases) = zoom out (distance increases)
        
        if (distanceFromMan < MIN_ZOOM) distanceFromMan = MIN_ZOOM;
        if (distanceFromMan > MAX_ZOOM) distanceFromMan = MAX_ZOOM;
        
        lastMouseY = y;
    }
    glutPostRedisplay();
}

// --- Main Function ---

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    glutCreateWindow("Enhanced 3D Autumn Scene - Realistic Walk & Zoom");
    
    initialize();
    
    glutDisplayFunc(renderScene);
    glutReshapeFunc(reshape);
    
    // Register ALL input handlers
    glutKeyboardFunc(keyboardInput);
    glutSpecialFunc(specialKeyInput); // New: for arrow keys
    glutMouseFunc(mouseInput);
    glutMotionFunc(mouseMove);
    
    glutTimerFunc(16, updateScene, 0);
    
    cout << "--- Enhanced 3D Autumn Scene Controls ---" << endl;
    cout << "Movement: WASD or Left/Right Arrow Keys" << endl;
    cout << "Zoom: Up/Down Arrow Keys or Right Click + Drag Vertically" << endl;
    cout << "Rotate: Left Click + Drag Horizontally" << endl;
    cout << "Features: Realistic Man Walking Animation, Fog, Dynamic Colors" << endl;

    glutMainLoop();
    return 0;
}
