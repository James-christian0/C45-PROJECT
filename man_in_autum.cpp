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

#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE 0x809D
#endif

using namespace std;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// --- Global Constants ---
const int WINDOW_WIDTH = 1200;
const int WINDOW_HEIGHT = 800;
const int NUM_LEAVES = 500; // Increased for more atmosphere

// --- Camera & Interaction Globals ---
float manPositionX = 0.0f;
float manPositionZ = 0.0f;
float cameraAngle = 45.0f;
float cameraPitch = -20.0f;
float distanceFromMan = 300.0f;

float targetCameraAngle = 45.0f;
float targetCameraPitch = -20.0f;
float targetDistanceFromMan = 300.0f;
const float CAMERA_SMOOTHNESS = 0.15f;

const float MIN_ZOOM = 50.0f;
const float MAX_ZOOM = 500.0f;

int lastMouseX = 0;
int lastMouseY = 0;
bool isRotating = false;
bool isZooming = false;
bool topDownView = false;

// --- Animation Globals ---
float leafDriftSpeed = 0.0f;
float jacketColor[3] = {0.8f, 0.2f, 0.0f};
float walkPhase = 0.0f;
bool isManMoving = false;
float sunAngle = 0.3f;
float timeOfDay = 0.0f;

// --- Wind System ---
float windStrength = 0.0f;
float windDirection = 0.0f;
float windGustTimer = 0.0f;
const float WIND_CHANGE_RATE = 0.02f;

// --- Sky System ---
float skyColorTransition = 0.0f;
struct Cloud {
    float x, y, z;
    float size;
    float speed;
    float density;
};
vector<Cloud> clouds;

// --- Leaf Structure ---
struct Leaf {
    float x, y, z;
    float color[3];
    float size;
    float fallSpeed;
    float rotationSpeed;
    float rotation;
};
vector<Leaf> fallingLeaves;

// --- Ground Objects ---
struct Pumpkin {
    float x, z;
    float size;
    float rotation;
};
vector<Pumpkin> pumpkins;

struct Flower {
    float x, z;
    float color[3];
    float petalRotation;
};
vector<Flower> flowers;

struct LeafPile {
    float x, z;
    float size;
    float height;
};
vector<LeafPile> leafPiles;

// --- Background Elements ---
struct DistantTree {
    float x, z;
    float height;
    float width;
};
vector<DistantTree> distantTrees;

struct Hill {
    float x, z;
    float radius;
    float height;
    bool isMountain; // NEW: distinguish mountains from hills
};
vector<Hill> hills;

// --- Textures ---
GLuint barkTexture;
GLuint groundTexture;

// --- Utility Functions ---

void setMaterialColor(float r, float g, float b) {
    GLfloat ambient[] = { r * 0.3f, g * 0.3f, b * 0.3f, 1.0f };
    GLfloat diffuse[] = { r, g, b, 1.0f };
    GLfloat specular[] = { 0.3f, 0.3f, 0.3f, 1.0f };
    glMaterialfv(GL_FRONT, GL_AMBIENT, ambient);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR, specular);
    glMaterialf(GL_FRONT, GL_SHININESS, 32.0f);
    glColor3f(r, g, b);
}

void drawCylinder(float baseRadius, float topRadius, float height) {
    GLUquadricObj *quadric = gluNewQuadric();
    gluQuadricNormals(quadric, GLU_SMOOTH);
    gluCylinder(quadric, baseRadius, topRadius, height, 24, 1);
    gluDeleteQuadric(quadric);
}

// IMPROVED: Higher quality bark texture
GLuint createBarkTexture() {
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    
    const int SIZE = 512; // Increased from 256
    unsigned char data[SIZE * SIZE * 3];
    
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            int idx = (i * SIZE + j) * 3;
            float verticalPattern = sin(j * 0.15f) * 25.0f;
            float horizontalPattern = sin(i * 0.05f) * 15.0f;
            float noise = (rand() % 30 - 15);
            float detail = verticalPattern + horizontalPattern + noise;
            
            data[idx] = 55 + detail;
            data[idx+1] = 35 + detail;
            data[idx+2] = 15 + detail;
        }
    }
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SIZE, SIZE, 0, 
                 GL_RGB, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    
    gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, SIZE, SIZE, 
                      GL_RGB, GL_UNSIGNED_BYTE, data);
    
    return texture;
}

// IMPROVED: Higher quality ground texture
GLuint createGroundTexture() {
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    
    const int SIZE = 512; // Increased from 256
    unsigned char data[SIZE * SIZE * 3];
    
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            int idx = (i * SIZE + j) * 3;
            float grassPattern = sin(i * 0.3f) * sin(j * 0.3f) * 10.0f;
            float variation = (rand() % 40 - 20);
            
            data[idx] = 35 + grassPattern + variation;
            data[idx+1] = 65 + grassPattern + variation;
            data[idx+2] = 15 + variation;
        }
    }
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SIZE, SIZE, 0, 
                 GL_RGB, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    
    gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, SIZE, SIZE, 
                      GL_RGB, GL_UNSIGNED_BYTE, data);
    
    return texture;
}

void initializeLeaves() {
    srand(time(0));
    fallingLeaves.clear();

    for (int i = 0; i < NUM_LEAVES; ++i) {
        Leaf l;
        l.x = (rand() % 1000) - 500.0f;
        l.y = (rand() % 400) + 100.0f;
        l.z = (rand() % 1000) - 500.0f;

        float r = static_cast <float> (rand()) / RAND_MAX;
        if (r < 0.25f) { l.color[0] = 0.8f; l.color[1] = 0.2f; l.color[2] = 0.0f; }
        else if (r < 0.50f) { l.color[0] = 1.0f; l.color[1] = 0.5f; l.color[2] = 0.0f; }
        else if (r < 0.75f) { l.color[0] = 1.0f; l.color[1] = 0.8f; l.color[2] = 0.0f; }
        else { l.color[0] = 0.6f; l.color[1] = 0.3f; l.color[2] = 0.1f; }

        l.size = 1.5f + (static_cast <float> (rand() % 100) / 100.0f) * 1.5f;
        l.fallSpeed = 0.3f + (static_cast <float> (rand() % 100) / 100.0f) * 1.0f;
        l.rotationSpeed = (rand() % 100) / 50.0f - 1.0f;
        l.rotation = rand() % 360;
        
        fallingLeaves.push_back(l);
    }
    
    pumpkins.clear();
    for (int i = 0; i < 25; ++i) {
        Pumpkin p;
        p.x = (rand() % 800) - 400.0f;
        p.z = (rand() % 800) - 400.0f;
        p.size = 12.0f + (rand() % 100) / 100.0f * 12.0f;
        p.rotation = rand() % 360;
        pumpkins.push_back(p);
    }
    
    flowers.clear();
    for (int i = 0; i < 40; ++i) {
        Flower f;
        f.x = (rand() % 800) - 400.0f;
        f.z = (rand() % 800) - 400.0f;
        f.petalRotation = rand() % 360;
        
        float colorChoice = static_cast <float> (rand()) / RAND_MAX;
        if (colorChoice < 0.3f) { f.color[0] = 1.0f; f.color[1] = 0.8f; f.color[2] = 0.0f; }
        else if (colorChoice < 0.5f) { f.color[0] = 1.0f; f.color[1] = 0.5f; f.color[2] = 0.0f; }
        else if (colorChoice < 0.7f) { f.color[0] = 0.9f; f.color[1] = 0.3f; f.color[2] = 0.2f; }
        else { f.color[0] = 0.8f; f.color[1] = 0.6f; f.color[2] = 0.9f; }
        
        flowers.push_back(f);
    }
    
    leafPiles.clear();
    for (int i = 0; i < 35; ++i) {
        LeafPile lp;
        lp.x = (rand() % 800) - 400.0f;
        lp.z = (rand() % 800) - 400.0f;
        lp.size = 20.0f + (rand() % 100) / 100.0f * 25.0f;
        lp.height = 4.0f + (rand() % 100) / 100.0f * 6.0f;
        leafPiles.push_back(lp);
    }
    
    // Enhanced cloud system
    clouds.clear();
    for (int i = 0; i < 35; ++i) {
        Cloud c;
        c.x = (rand() % 3000) - 1500.0f;
        c.y = 250.0f + (rand() % 250);
        c.z = (rand() % 3000) - 1500.0f;
        c.size = 35.0f + (rand() % 100) / 100.0f * 70.0f;
        c.speed = 0.2f + (rand() % 100) / 100.0f * 0.5f;
        c.density = 0.7f + (rand() % 100) / 300.0f;
        clouds.push_back(c);
    }
    
    // Background hills
    hills.clear();
    for (int i = 0; i < 12; ++i) {
        Hill h;
        h.x = (rand() % 3000) - 1500.0f;
        h.z = -800.0f - (rand() % 1000);
        h.radius = 200.0f + (rand() % 300);
        h.height = 80.0f + (rand() % 120);
        h.isMountain = false;
        hills.push_back(h);
    }
    
    // NEW: Add surrounding mountains in a circle
    const int numMountains = 24; // Mountains around the perimeter
    const float mountainDistance = 1800.0f;
    for (int i = 0; i < numMountains; i++) {
        Hill mountain;
        float angle = (i * 2.0f * M_PI) / numMountains;
        mountain.x = mountainDistance * cos(angle);
        mountain.z = mountainDistance * sin(angle);
        mountain.radius = 250.0f + (rand() % 200);
        mountain.height = 300.0f + (rand() % 250); // Much taller
        mountain.isMountain = true;
        hills.push_back(mountain);
    }
    
    // Distant trees
    distantTrees.clear();
    for (int i = 0; i < 60; ++i) {
        DistantTree dt;
        dt.x = (rand() % 2000) - 1000.0f;
        dt.z = -600.0f - (rand() % 800);
        dt.height = 60.0f + (rand() % 80);
        dt.width = 30.0f + (rand() % 40);
        distantTrees.push_back(dt);
    }
}

// IMPROVED: Better ground with more detail - FIXED winding order
void drawGround() {
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, groundTexture);
    setMaterialColor(0.25f, 0.55f, 0.15f);
    
    const int gridSize = 60;
    const float cellSize = 80.0f;
    
    for (int i = 0; i < gridSize; i++) {
        glBegin(GL_TRIANGLE_STRIP);
        for (int j = 0; j <= gridSize; j++) {
            float x1 = (i - gridSize/2) * cellSize;
            float x2 = (i + 1 - gridSize/2) * cellSize;
            float z = (j - gridSize/2) * cellSize;
            
            float h1 = 3.0f * sin(x1 * 0.008f + z * 0.008f) + 
                      1.5f * cos(x1 * 0.02f) * sin(z * 0.015f);
            float h2 = 3.0f * sin(x2 * 0.008f + z * 0.008f) + 
                      1.5f * cos(x2 * 0.02f) * sin(z * 0.015f);
            
            // Fixed: Swapped vertex order and flipped normal
            glNormal3f(0, 1, 0);
            glTexCoord2f((i+1) * 0.3f, j * 0.3f);
            glVertex3f(x2, h2, z);
            glTexCoord2f(i * 0.3f, j * 0.3f);
            glVertex3f(x1, h1, z);
        }
        glEnd();
    }
    
    glDisable(GL_TEXTURE_2D);
}

// Draw distant hills for background
void drawHills() {
    for (const auto& hill : hills) {
        glPushMatrix();
        glTranslatef(hill.x, 0, hill.z);
        
        if (hill.isMountain) {
            // Mountains are darker and more dramatic
            setMaterialColor(0.35f + (rand() % 15) / 100.0f, 
                            0.4f + (rand() % 15) / 100.0f, 
                            0.25f);
        } else {
            // Regular hills
            setMaterialColor(0.4f + (rand() % 20) / 100.0f, 
                            0.5f + (rand() % 20) / 100.0f, 
                            0.2f);
        }
        
        glScalef(hill.radius, hill.height, hill.radius);
        glutSolidSphere(1.0f, 20, 12);
        
        glPopMatrix();
    }
}

// Draw simplified distant trees
void drawDistantTree(float x, float z, float height, float width) {
    glPushMatrix();
    glTranslatef(x, 0, z);
    
    // Trunk
    setMaterialColor(0.3f, 0.2f, 0.1f);
    glPushMatrix();
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
    drawCylinder(width * 0.15f, width * 0.12f, height * 0.4f);
    glPopMatrix();
    
    // Autumn foliage
    glPushMatrix();
    glTranslatef(0, height * 0.5f, 0);
    setMaterialColor(0.7f + (rand() % 20) / 100.0f, 
                    0.4f + (rand() % 20) / 100.0f, 
                    0.1f);
    glScalef(width, height * 0.6f, width);
    glutSolidSphere(1.0f, 12, 12);
    glPopMatrix();
    
    glPopMatrix();
}

void draw3DMan(float x, float y, float z) {
    glPushMatrix();
    glTranslatef(x, y, z);
    glRotatef(90.0f, 0.0f, 1.0f, 0.0f);

    const float MAN_HEIGHT = 100.0f;
    const float TORSO_HEIGHT = MAN_HEIGHT * 0.45f;
    const float LEG_LENGTH = MAN_HEIGHT * 0.45f;
    const float ARM_LENGTH = MAN_HEIGHT * 0.40f;
    const float BODY_RADIUS = 12.0f;
    const float LIMB_RADIUS = 5.0f;

    setMaterialColor(1.0f, 0.8f, 0.7f);
    glPushMatrix();
    glTranslatef(0.0f, TORSO_HEIGHT + LEG_LENGTH + 10.0f, 0.0f);
    glutSolidSphere(10.0f, 20, 20);
    glPopMatrix();

    setMaterialColor(jacketColor[0], jacketColor[1], jacketColor[2]);
    glPushMatrix();
    glTranslatef(0.0f, LEG_LENGTH, 0.0f);
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
    drawCylinder(BODY_RADIUS, BODY_RADIUS * 0.8f, TORSO_HEIGHT);
    glPopMatrix();

    float armAngle = 20.0f * sin(walkPhase);
    setMaterialColor(jacketColor[0] * 0.8f, jacketColor[1] * 0.8f, jacketColor[2] * 0.8f);

    for (int i = -1; i <= 1; i += 2) {
        glPushMatrix();
        glTranslatef(i * BODY_RADIUS, LEG_LENGTH + TORSO_HEIGHT * 0.8f, 0.0f);
        glRotatef(i * armAngle, 1.0f, 0.0f, 0.0f);
        glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
        drawCylinder(LIMB_RADIUS, LIMB_RADIUS * 0.8f, ARM_LENGTH);
        
        glPushMatrix();
        glTranslatef(0.0f, 0.0f, ARM_LENGTH);
        glutSolidSphere(LIMB_RADIUS * 0.8f, 12, 12);
        glPopMatrix();
        
        glPopMatrix();
    }

    float legAngle = 30.0f * sin(walkPhase);
    setMaterialColor(0.1f, 0.1f, 0.5f);

    for (int i = -1; i <= 1; i += 2) {
        glPushMatrix();
        glTranslatef(i * LIMB_RADIUS, LEG_LENGTH, 0.0f);
        glRotatef(i * legAngle, 1.0f, 0.0f, 0.0f);
        glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
        drawCylinder(LIMB_RADIUS + 2.0f, LIMB_RADIUS, LEG_LENGTH);
        glPopMatrix();
    }

    glPopMatrix();
}

// NEW: Enhanced sky with gradient
void drawEnhancedSky() {
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    
    glBegin(GL_QUADS);
    
    float timeInfluence = sin(timeOfDay * 0.5f);
    float topR = 0.6f + 0.15f * timeInfluence;
    float topG = 0.7f + 0.1f * timeInfluence;
    float topB = 0.85f + 0.1f * timeInfluence;
    
    float botR = 0.95f + 0.05f * timeInfluence;
    float botG = 0.85f + 0.05f * timeInfluence;
    float botB = 0.7f;
    
    glColor3f(topR, topG, topB);
    glVertex3f(-1.0f, 1.0f, -0.99f);
    glVertex3f(1.0f, 1.0f, -0.99f);
    
    glColor3f(botR, botG, botB);
    glVertex3f(1.0f, -0.3f, -0.99f);
    glVertex3f(-1.0f, -0.3f, -0.99f);
    glEnd();
    
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
}

// IMPROVED: Better cloud rendering
void drawCloud(float x, float y, float z, float size, float density) {
    glPushMatrix();
    glTranslatef(x, y, z);
    
    float cloudR = 0.95f - (1.0f - density) * 0.15f;
    float cloudG = 0.93f - (1.0f - density) * 0.15f;
    float cloudB = 0.90f - (1.0f - density) * 0.10f;
    setMaterialColor(cloudR, cloudG, cloudB);
    
    glutSolidSphere(size * density, 20, 20);
    
    glTranslatef(size * 0.6f, size * 0.15f, size * 0.1f);
    glutSolidSphere(size * 0.85f * density, 18, 18);
    
    glTranslatef(-size * 1.3f, size * 0.1f, -size * 0.2f);
    glutSolidSphere(size * 0.75f * density, 16, 16);
    
    glTranslatef(size * 0.7f, -size * 0.35f, size * 0.35f);
    glutSolidSphere(size * 0.65f * density, 14, 14);
    
    glTranslatef(0, size * 0.25f, -size * 0.7f);
    glutSolidSphere(size * 0.55f * density, 12, 12);
    
    glTranslatef(-size * 0.3f, -size * 0.2f, size * 0.4f);
    glutSolidSphere(size * 0.4f * density, 10, 10);
    
    glPopMatrix();
}

// IMPROVED: Enhanced sun with glow
void drawSun(float x, float y, float z) {
    glPushMatrix();
    glTranslatef(x, y, z);
    
    // Main sun body
    setMaterialColor(1.0f, 0.85f, 0.5f);
    glutSolidSphere(60.0f, 32, 32);
    
    // Multiple glow layers
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(GL_FALSE);
    
    setMaterialColor(1.0f, 0.9f, 0.7f);
    glutSolidSphere(75.0f, 24, 24);
    
    setMaterialColor(1.0f, 0.85f, 0.6f);
    glutSolidSphere(95.0f, 20, 20);
    
    setMaterialColor(1.0f, 0.8f, 0.5f);
    glutSolidSphere(120.0f, 16, 16);
    
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    
    glPopMatrix();
}

void drawDynamicSky() {
    // Draw enhanced sky gradient
    drawEnhancedSky();
    
    // Draw sun - MUCH HIGHER in the sky
    float sunX = 1200.0f * cos(sunAngle);
    float sunY = 600.0f + 400.0f * sin(sunAngle); // Raised from 300 + 250
    float sunZ = 1200.0f * sin(sunAngle);
    drawSun(sunX, sunY, sunZ);
    
    // Draw clouds
    for (const auto& cloud : clouds) {
        drawCloud(cloud.x, cloud.y, cloud.z, cloud.size, cloud.density);
    }
}

// IMPROVED: Higher polygon count trees
void draw3DTree(float x, float z) {
    glPushMatrix();
    glTranslatef(x, 0.0f, z);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, barkTexture);
    setMaterialColor(0.35f, 0.25f, 0.15f);
    
    GLUquadricObj *quadric = gluNewQuadric();
    gluQuadricTexture(quadric, GL_TRUE);
    gluQuadricNormals(quadric, GLU_SMOOTH);
    
    glPushMatrix();
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
    gluCylinder(quadric, 15.0f, 10.0f, 120.0f, 32, 16);
    glPopMatrix();
    
    gluDeleteQuadric(quadric);
    glDisable(GL_TEXTURE_2D);

    glPushMatrix();
    glTranslatef(0.0f, 120.0f, 0.0f);
    
    setMaterialColor(0.75f, 0.35f, 0.05f);
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
    glutSolidCone(60.0f, 70.0f, 24, 24);
    
    glTranslatef(0.0f, 0.0f, 35.0f);
    setMaterialColor(0.85f, 0.55f, 0.1f);
    glutSolidCone(50.0f, 70.0f, 24, 24);

    glPopMatrix();
    glPopMatrix();
}

void draw3DLeaf(float x, float y, float z, const float color[3], float size, float rotation) {
    glPushMatrix();
    glTranslatef(x, y, z);
    
    glRotatef(rotation, 0, 1, 0);
    glRotatef(sin(rotation * 0.1f) * 30, 1, 0, 0);
    
    setMaterialColor(color[0], color[1], color[2]);
    
    glBegin(GL_TRIANGLES);
    
    glNormal3f(0, 0.7f, 0.3f);
    glVertex3f(0, 0, 0);
    glVertex3f(-size, size * 0.5f, 0);
    glVertex3f(0, size * 1.2f, 0);
    
    glVertex3f(0, 0, 0);
    glVertex3f(0, size * 1.2f, 0);
    glVertex3f(size, size * 0.5f, 0);
    
    glEnd();
    
    setMaterialColor(color[0] * 0.7f, color[1] * 0.7f, color[2] * 0.7f);
    glBegin(GL_LINES);
    glVertex3f(0, 0, 0);
    glVertex3f(0, -size * 0.3f, 0);
    glEnd();
    
    glPopMatrix();
}

void draw3DLeaves() {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    for (const auto& leaf : fallingLeaves) {
        float windEffectX = windStrength * 30.0f * cos(windDirection);
        float windEffectZ = windStrength * 30.0f * sin(windDirection);
        
        float currentX = leaf.x + 20.0f * sin(leafDriftSpeed + leaf.z * 0.1f) + windEffectX;
        float currentY = leaf.y + 5.0f * cos(leafDriftSpeed * 2.0f + leaf.x * 0.1f);
        float currentZ = leaf.z + windEffectZ;
        
        draw3DLeaf(currentX, currentY, currentZ, leaf.color, leaf.size, leaf.rotation);
    }
    
    glDisable(GL_BLEND);
}

void drawDetailedPumpkin(float x, float z, float size, float rotation) {
    glPushMatrix();
    glTranslatef(x, size * 1.1f, z);  // Raised even higher - now 1.1f * size to be clearly above ground
    glRotatef(rotation, 0.0f, 1.0f, 0.0f);
    
    const int segments = 40; // Even more segments for ultra-smooth surface
    const int ridges = 14;   // More ridges for better detail
    
    // Draw each ridge as a vertical section
    for (int r = 0; r < ridges; r++) {
        float angle1 = (r * 2.0f * M_PI) / ridges;
        float angle2 = ((r + 1) * 2.0f * M_PI) / ridges;
        
        // Alternate colors with more variation for depth
        float colorVar = 0.05f * sin(r * 0.5f);
        if (r % 2 == 0) {
            setMaterialColor(1.0f, 0.5f + colorVar, 0.05f);
        } else {
            setMaterialColor(0.95f, 0.45f + colorVar, 0.02f);
        }
        
        glBegin(GL_QUAD_STRIP);
        for (int s = 0; s <= segments; s++) {
            float v = (float)s / segments;
            
            // Create pumpkin shape - flatter at top and bottom
            float yNormalized = v * 2.0f - 1.0f; // -1 to 1
            float yPos = yNormalized * size * 0.85f;
            
            // Enhanced pumpkin profile curve - more natural bulge
            float heightFactor = 1.0f - yNormalized * yNormalized; // parabola
            heightFactor = pow(heightFactor, 0.55f); // slightly rounder
            
            // More pronounced ridge depth variation
            float ridgeAngle = (r + 0.5f) * 2.0f * M_PI / ridges;
            float ridgeDepth = 0.85f + 0.15f * cos(ridgeAngle * ridges * 0.5f);
            
            // Calculate radius with pumpkin curve
            float baseRadius = size * heightFactor * ridgeDepth;
            
            // Enhanced bulge for each ridge with more detail
            float ridgeInfluence = 0.08f * sin(v * M_PI);
            baseRadius += ridgeInfluence * size;
            
            // Add subtle wave pattern on ridges
            float waveDetail = 0.02f * size * sin(v * M_PI * 4.0f);
            baseRadius += waveDetail;
            
            float x1 = baseRadius * cos(angle1);
            float z1 = baseRadius * sin(angle1);
            float x2 = baseRadius * cos(angle2);
            float z2 = baseRadius * sin(angle2);
            
            // Calculate proper normals for better lighting
            float nx1 = cos(angle1) * heightFactor;
            float nz1 = sin(angle1) * heightFactor;
            float nx2 = cos(angle2) * heightFactor;
            float nz2 = sin(angle2) * heightFactor;
            float ny = -yNormalized * 0.4f;
            
            glNormal3f(nx1, ny, nz1);
            glVertex3f(x1, yPos, z1);
            glNormal3f(nx2, ny, nz2);
            glVertex3f(x2, yPos, z2);
        }
        glEnd();
    }
    
    // Bottom cap (more detailed and flattened)
    setMaterialColor(0.85f, 0.38f, 0.0f);
    glBegin(GL_TRIANGLE_FAN);
    glNormal3f(0, -1, 0);
    glVertex3f(0, -size * 0.85f, 0);
    for (int i = 0; i <= ridges * 2; i++) {
        float angle = (i * 2.0f * M_PI) / (ridges * 2);
        float bottomRadius = size * 0.35f;
        // Add slight wave to bottom edge
        float edgeWave = 0.03f * size * sin(i * M_PI / ridges);
        glVertex3f((bottomRadius + edgeWave) * cos(angle), -size * 0.85f, (bottomRadius + edgeWave) * sin(angle));
    }
    glEnd();
    
    // Top cap (where stem connects) - with more detail
    setMaterialColor(0.88f, 0.42f, 0.01f);
    glBegin(GL_TRIANGLE_FAN);
    glNormal3f(0, 1, 0);
    glVertex3f(0, size * 0.85f, 0);
    for (int i = 0; i <= ridges * 2; i++) {
        float angle = (i * 2.0f * M_PI) / (ridges * 2);
        float topRadius = size * 0.28f;
        // Add indentation pattern around stem
        float indent = 0.02f * size * sin(i * M_PI / ridges * 2.0f);
        glVertex3f((topRadius - indent) * cos(angle), size * 0.85f, (topRadius - indent) * sin(angle));
    }
    glEnd();
    
    // Enhanced stem with much more detail
    glPushMatrix();
    glTranslatef(0, size * 0.85f, 0);
    
    // Stem base ring (decorative detail where stem meets pumpkin)
    setMaterialColor(0.32f, 0.42f, 0.09f);
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
    glutSolidTorus(size * 0.04f, size * 0.18f, 8, 16);
    
    // Stem base (slightly wider and textured)
    setMaterialColor(0.35f, 0.45f, 0.1f);
    drawCylinder(size * 0.16f, size * 0.13f, size * 0.18f);
    
    // Main stem section 1 (curved)
    glTranslatef(0, 0, size * 0.18f);
    glRotatef(10.0f, 0.0f, 1.0f, 0.0f);
    glRotatef(3.0f, 1.0f, 0.0f, 0.0f);
    setMaterialColor(0.3f, 0.5f, 0.12f);
    drawCylinder(size * 0.13f, size * 0.10f, size * 0.25f);
    
    // Add texture bumps on main stem
    for (int i = 0; i < 4; i++) {
        glPushMatrix();
        glTranslatef(0, 0, size * 0.06f * i);
        setMaterialColor(0.28f, 0.46f, 0.10f);
        glutSolidTorus(size * 0.015f, size * 0.11f, 6, 12);
        glPopMatrix();
    }
    
    // Main stem section 2 (continues curve)
    glTranslatef(0, 0, size * 0.25f);
    glRotatef(12.0f, 0.0f, 1.0f, 0.0f);
    glRotatef(5.0f, 1.0f, 0.0f, 0.0f);
    setMaterialColor(0.29f, 0.49f, 0.11f);
    drawCylinder(size * 0.10f, size * 0.07f, size * 0.22f);
    
    // Stem top section (tapers to point)
    glTranslatef(0, 0, size * 0.22f);
    glRotatef(8.0f, 1.0f, 0.0f, 0.0f);
    setMaterialColor(0.28f, 0.48f, 0.1f);
    drawCylinder(size * 0.07f, size * 0.03f, size * 0.15f);
    
    glPopMatrix();
    
    // Add decorative grooves/ridges on the stem
    glPushMatrix();
    glTranslatef(0, size * 0.85f, 0);
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
    setMaterialColor(0.25f, 0.4f, 0.08f);
    for (int i = 0; i < 5; i++) {
        glPushMatrix();
        glTranslatef(0, 0, size * 0.22f + i * size * 0.11f);
        glutSolidTorus(size * 0.018f, size * 0.09f - i * size * 0.01f, 6, 12);
        glPopMatrix();
    }
    glPopMatrix();
    
    // Add small leaf details on stem
    for (int i = 0; i < 2; i++) {
        glPushMatrix();
        glTranslatef(0, size * (0.85f + 0.3f + i * 0.25f), 0);
        glRotatef(i * 120.0f, 0.0f, 1.0f, 0.0f);
        glTranslatef(size * 0.12f, 0, 0);
        glRotatef(-45.0f, 0.0f, 0.0f, 1.0f);
        
        // Small leaf shape
        setMaterialColor(0.25f, 0.55f, 0.15f);
        glBegin(GL_TRIANGLES);
        glNormal3f(0, 0, 1);
        glVertex3f(0, 0, 0);
        glVertex3f(-size * 0.08f, size * 0.06f, 0);
        glVertex3f(0, size * 0.12f, 0);
        
        glVertex3f(0, 0, 0);
        glVertex3f(0, size * 0.12f, 0);
        glVertex3f(size * 0.08f, size * 0.06f, 0);
        glEnd();
        
        glPopMatrix();
    }
    
    glPopMatrix();
}

void drawChrysanthemum(float x, float z, float r, float g, float b, float rotation) {
    glPushMatrix();
    glTranslatef(x, 0.0f, z);
    
    setMaterialColor(0.2f, 0.6f, 0.2f);
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
    drawCylinder(0.5f, 0.3f, 8.0f);
    glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
    
    glTranslatef(0.0f, 8.5f, 0.0f);
    setMaterialColor(1.0f, 0.9f, 0.0f);
    glutSolidSphere(2.5f, 12, 12);
    
    setMaterialColor(r, g, b);
    for (int i = 0; i < 8; ++i) {
        float angle = (i * 45.0f + rotation) * M_PI / 180.0f;
        glPushMatrix();
        glTranslatef(4.0f * cos(angle), 0.0f, 4.0f * sin(angle));
        glScalef(2.0f, 0.4f, 1.2f);
        glutSolidSphere(1.5f, 10, 10);
        glPopMatrix();
    }
    
    glPopMatrix();
}

void drawLeafPile(float x, float z, float size, float height) {
    glPushMatrix();
    glTranslatef(x, 0.1f, z);
    
    int numLeafsInPile = 8;
    for (int i = 0; i < numLeafsInPile; ++i) {
        float offsetX = (rand() % 100 - 50) / 50.0f * size * 0.3f;
        float offsetZ = (rand() % 100 - 50) / 50.0f * size * 0.3f;
        float offsetY = (rand() % 100) / 100.0f * height * 0.5f;
        
        float colorRand = static_cast <float> (rand()) / RAND_MAX;
        if (colorRand < 0.33f) setMaterialColor(0.8f, 0.3f, 0.0f);
        else if (colorRand < 0.66f) setMaterialColor(1.0f, 0.6f, 0.0f);
        else setMaterialColor(0.6f, 0.4f, 0.1f);
        
        glPushMatrix();
        glTranslatef(offsetX, offsetY, offsetZ);
        glScalef(size * 0.3f, height * 0.3f, size * 0.3f);
        glutSolidSphere(1.0f, 12, 12);
        glPopMatrix();
    }
    
    glPopMatrix();
}

void reshape(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, (GLfloat)w / (GLfloat)h, 1.0, 6000.0);
    glMatrixMode(GL_MODELVIEW);
}

void initialize() {
    // IMPROVED: Enable antialiasing
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_POLYGON_SMOOTH);
    glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
    
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    glEnable(GL_CULL_FACE);
    
    glClearColor(0.8f, 0.7f, 0.6f, 1.0f);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
    
    glEnable(GL_FOG);
    glFogi(GL_FOG_MODE, GL_EXP2);
    GLfloat fogColor[4] = {0.8f, 0.7f, 0.6f, 1.0f};
    glFogfv(GL_FOG_COLOR, fogColor);
    glFogf(GL_FOG_DENSITY, 0.00015f);

    glShadeModel(GL_SMOOTH);
    
    barkTexture = createBarkTexture();
    groundTexture = createGroundTexture();
    
    initializeLeaves();
}

void renderScene() {
    // Autumn sky colors - warmer tones
    float timeInfluence = sin(timeOfDay * 0.5f);
    float skyR = 0.75f + 0.15f * timeInfluence;
    float skyG = 0.65f + 0.1f * timeInfluence;
    float skyB = 0.55f + 0.05f * timeInfluence;
    glClearColor(skyR, skyG, skyB, 1.0f);
    
    // Update fog color to match sky
    GLfloat fogColor[4] = {skyR, skyG, skyB, 1.0f};
    glFogfv(GL_FOG_COLOR, fogColor);
    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    float camX, camY, camZ;
    
    if (topDownView) {
        camX = manPositionX;
        camY = 400.0f;
        camZ = manPositionZ;
        gluLookAt(camX, camY, camZ,
                  manPositionX, 0.0f, manPositionZ,
                  0.0f, 0.0f, -1.0f);
    } else {
        float pitchRad = cameraPitch * M_PI / 180.0f;
        float angleRad = cameraAngle * M_PI / 180.0f;
        
        camX = manPositionX + distanceFromMan * sin(angleRad) * cos(pitchRad);
        camY = 100.0f + distanceFromMan * sin(pitchRad);
        camZ = manPositionZ + distanceFromMan * cos(angleRad) * cos(pitchRad);
        
        gluLookAt(camX, camY, camZ,
                  manPositionX, 30.0f, manPositionZ,
                  0.0f, 1.0f, 0.0f);
    }

    // Autumn sun lighting - warmer and lower angle
    float sunX = 1200.0f * cos(sunAngle);
    float sunY = 600.0f + 400.0f * sin(sunAngle); // Match the raised sun position
    float sunZ = 1200.0f * sin(sunAngle);
    
    GLfloat light_position[] = { sunX, sunY, sunZ, 0.0f };
    GLfloat light_ambient[] = { 0.45f, 0.4f, 0.35f, 1.0f };
    GLfloat light_diffuse[] = { 1.0f, 0.88f, 0.65f, 1.0f };
    GLfloat light_specular[] = { 0.9f, 0.8f, 0.6f, 1.0f };
    
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);

    // Draw background elements first
    drawHills();
    
    // Draw distant trees
    for (const auto& dt : distantTrees) {
        drawDistantTree(dt.x, dt.z, dt.height, dt.width);
    }
    
    drawGround();
    drawDynamicSky();
    
    for (const auto& pile : leafPiles) {
        drawLeafPile(pile.x, pile.z, pile.size, pile.height);
    }
    
    for (const auto& pumpkin : pumpkins) {
        drawDetailedPumpkin(pumpkin.x, pumpkin.z, pumpkin.size, pumpkin.rotation);
    }
    
    for (const auto& flower : flowers) {
        drawChrysanthemum(flower.x, flower.z, flower.color[0], flower.color[1], flower.color[2], flower.petalRotation);
    }
    
    // Draw foreground trees
    float treeSpacing = 180.0f;
    for (int i = -3; i <= 3; i++) {
        for (int j = -3; j <= 3; j++) {
            if (i == 0 && j == 0) continue;
            draw3DTree(i * treeSpacing, j * treeSpacing);
        }
    }
    
    draw3DMan(manPositionX, 0.0f, manPositionZ);
    draw3DLeaves();

    glutSwapBuffers();
}

void updateScene(int value) {
    // Smooth camera interpolation
    cameraAngle += (targetCameraAngle - cameraAngle) * CAMERA_SMOOTHNESS;
    cameraPitch += (targetCameraPitch - cameraPitch) * CAMERA_SMOOTHNESS;
    distanceFromMan += (targetDistanceFromMan - distanceFromMan) * CAMERA_SMOOTHNESS;
    
    // Update leaf positions with rotation
    for (auto& leaf : fallingLeaves) {
        leaf.y -= leaf.fallSpeed;
        leaf.rotation += leaf.rotationSpeed;
        
        if (leaf.y < 0) {
            leaf.y = 500.0f + (rand() % 100);
            leaf.x = (rand() % 1000) - 500.0f;
            leaf.z = (rand() % 1000) - 500.0f;
            leaf.fallSpeed = 0.3f + (static_cast <float> (rand() % 100) / 100.0f) * 1.0f;
            leaf.rotation = rand() % 360;
        }
    }
    
    // Update clouds
    for (auto& cloud : clouds) {
        cloud.x += cloud.speed;
        if (cloud.x > 1500.0f) {
            cloud.x = -1500.0f;
            cloud.z = (rand() % 3000) - 1500.0f;
        }
    }

    leafDriftSpeed += 0.02f;
    sunAngle += 0.003f;
    if (sunAngle > 2.0f * M_PI) sunAngle -= 2.0f * M_PI;
    
    timeOfDay += 0.005f;

    if (isManMoving) {
        walkPhase = fmod(walkPhase + 0.3f, 2.0f * M_PI);
    } else {
        if (walkPhase > 0.1f) {
            walkPhase *= 0.8f;
        } else {
            walkPhase = 0.0f;
        }
    }
    isManMoving = false;

    // Jacket color cycle
    static float hue = 0.0f;
    hue = fmod(hue + 0.002f, 1.0f);
    if (hue < 0.333f) { 
        jacketColor[0] = 1.0f; 
        jacketColor[1] = hue * 3.0f; 
        jacketColor[2] = 0.0f; 
    } else if (hue < 0.666f) { 
        jacketColor[0] = 1.0f - (hue - 0.333f) * 3.0f; 
        jacketColor[1] = 1.0f; 
        jacketColor[2] = (hue - 0.333f) * 3.0f; 
    } else { 
        jacketColor[0] = (hue - 0.666f) * 3.0f; 
        jacketColor[1] = 1.0f - (hue - 0.666f) * 3.0f; 
        jacketColor[2] = 1.0f; 
    }

    glutPostRedisplay();
    glutTimerFunc(16, updateScene, 0);
}

void keyboardInput(unsigned char key, int x, int y) {
    float move_step = 5.0f;
    
    if (key == 'a' || key == 'A') { manPositionX -= move_step; isManMoving = true; } 
    else if (key == 'd' || key == 'D') { manPositionX += move_step; isManMoving = true; } 
    else if (key == 'w' || key == 'W') { manPositionZ -= move_step; isManMoving = true; }
    else if (key == 's' || key == 'S') { manPositionZ += move_step; isManMoving = true; }
    else if (key == 'v' || key == 'V') { 
        topDownView = !topDownView;
        cout << "Top-down view: " << (topDownView ? "ON" : "OFF") << endl;
    }
    else if (key == 27) exit(0);
    
    if (manPositionX < -800.0f) manPositionX = -800.0f;
    if (manPositionX > 800.0f) manPositionX = 800.0f;
    if (manPositionZ < -800.0f) manPositionZ = -800.0f;
    if (manPositionZ > 800.0f) manPositionZ = 800.0f;

    glutPostRedisplay();
}

void specialKeyInput(int key, int x, int y) {
    float move_step = 5.0f;
    float zoom_step = 10.0f;

    switch (key) {
        case GLUT_KEY_LEFT:
            manPositionX -= move_step;
            isManMoving = true;
            break;
        case GLUT_KEY_RIGHT:
            manPositionX += move_step;
            isManMoving = true;
            break;
        case GLUT_KEY_UP:
            targetDistanceFromMan -= zoom_step;
            if (targetDistanceFromMan < MIN_ZOOM) targetDistanceFromMan = MIN_ZOOM;
            break;
        case GLUT_KEY_DOWN:
            targetDistanceFromMan += zoom_step;
            if (targetDistanceFromMan > MAX_ZOOM) targetDistanceFromMan = MAX_ZOOM;
            break;
    }
    
    glutPostRedisplay();
}

void mouseInput(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON) {
        isRotating = (state == GLUT_DOWN);
        if (state == GLUT_DOWN) {
            lastMouseX = x;
            lastMouseY = y;
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
        float rotationSpeed = 0.5f;
        targetCameraAngle -= (x - lastMouseX) * rotationSpeed;
        lastMouseX = x;
        
        float pitchSpeed = 0.5f;
        targetCameraPitch -= (y - lastMouseY) * pitchSpeed;
        
        if (targetCameraPitch > 60.0f) targetCameraPitch = 60.0f;
        if (targetCameraPitch < -80.0f) targetCameraPitch = -80.0f;
        lastMouseY = y;
    } else if (isZooming) {
        float zoomSpeed = 2.0f;
        targetDistanceFromMan += (y - lastMouseY) * zoomSpeed;
        
        if (targetDistanceFromMan < MIN_ZOOM) targetDistanceFromMan = MIN_ZOOM;
        if (targetDistanceFromMan > MAX_ZOOM) targetDistanceFromMan = MAX_ZOOM;
        
        lastMouseY = y;
    }
    glutPostRedisplay();
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_MULTISAMPLE);
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    glutCreateWindow("Enhanced Realistic 3D Autumn Scene - with Mountains");
    
    initialize();
    
    glutDisplayFunc(renderScene);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboardInput);
    glutSpecialFunc(specialKeyInput);
    glutMouseFunc(mouseInput);
    glutMotionFunc(mouseMove);
    glutTimerFunc(16, updateScene, 0);
    
    cout << "=== ENHANCED REALISTIC AUTUMN SCENE - WITH MOUNTAINS ===" << endl;
    cout << "\n--- CONTROLS ---" << endl;
    cout << "Movement: WASD or Arrow Keys (Left/Right)" << endl;
    cout << "Zoom: Up/Down Arrow Keys or Right Click + Drag" << endl;
    cout << "Rotate Camera: Left Click + Drag" << endl;
    cout << "Toggle Top-Down View: V key" << endl;
    cout << "ESC: Exit" << endl;
    cout << "\n--- NEW IMPROVEMENTS ---" << endl;
    cout << "? Sun positioned MUCH HIGHER in the sky" << endl;
    cout << "? 24 surrounding MOUNTAINS creating a valley" << endl;
    cout << "? Mountains are 300-550 units tall" << endl;
    cout << "? Scene feels enclosed by mountain ring" << endl;
    cout << "? ANTIALIASING enabled (smooth edges)" << endl;
    cout << "? Higher resolution textures (512x512)" << endl;
    cout << "? Enhanced sky gradient system" << endl;
    cout << "? Improved sun with multiple glow layers" << endl;
    cout << "? More detailed clouds (35 total)" << endl;
    cout << "? Higher polygon count on all 3D objects" << endl;
    cout << "? Better lighting and fog system" << endl;
    cout << "? 500 falling leaves with physics" << endl;
    cout << "? More pumpkins (25), flowers (40), leaf piles (35)" << endl;
    cout << "? 60 background trees for depth" << endl;
    cout << "? 12 distant hills + 24 mountains" << endl;
    cout << "? Extended viewing distance (6000 units)" << endl;

    glutMainLoop();
    return 0;
}
