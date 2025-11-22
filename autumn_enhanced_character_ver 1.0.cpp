#include <iostream>
#include <cmath>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <algorithm> 

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

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
const int NUM_LEAVES = 150; 

float manPositionX = 0.0f;
float manPositionZ = 0.0f;
float manRotationY = 0.0f; 
float cameraAngle = 45.0f;
float distanceFromMan = 350.0f; 
const float MIN_ZOOM = 100.0f;
const float MAX_ZOOM = 800.0f;

bool keyStates[256] = {false}; 
int lastMouseX = 0;
int lastMouseY = 0;
bool isRotating = false;
bool isZooming = false;

float leafDriftSpeed = 0.0f;
float walkPhase = 0.0f; 
bool isManMoving = false;
bool isSprinting = false;
bool isCrouching = false;

GLfloat lightPos[] = { 300.0f, 500.0f, 200.0f, 1.0f };

struct Leaf {
    float x, y, z;
    float color[3];
    float size;
    float fallSpeed;
};
vector<Leaf> fallingLeaves;

void setMaterialColor(float r, float g, float b) {
    GLfloat ambient[] = { r * 0.4f, g * 0.4f, b * 0.4f, 1.0f };
    GLfloat diffuse[] = { r, g, b, 1.0f };
    GLfloat specular[] = { 0.2f, 0.2f, 0.2f, 1.0f };
    GLfloat shininess[] = { 10.0f };

    glMaterialfv(GL_FRONT, GL_AMBIENT, ambient);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR, specular);
    glMaterialfv(GL_FRONT, GL_SHININESS, shininess);
    glColor3f(r, g, b);
}

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
        l.x = (rand() % 600) - 300.0f;
        l.y = (rand() % 400) + 100.0f;
        l.z = (rand() % 600) - 300.0f;
        
        float r = static_cast <float> (rand()) / RAND_MAX;
        if (r < 0.25f) { l.color[0] = 0.8f; l.color[1] = 0.2f; l.color[2] = 0.0f; } 
        else if (r < 0.50f) { l.color[0] = 1.0f; l.color[1] = 0.5f; l.color[2] = 0.0f; } 
        else if (r < 0.75f) { l.color[0] = 1.0f; l.color[1] = 1.0f; l.color[2] = 0.0f; } 
        else { l.color[0] = 0.5f; l.color[1] = 0.3f; l.color[2] = 0.1f; } 

        l.size = 2.0f + (static_cast <float> (rand() % 100) / 100.0f);
        l.fallSpeed = 0.5f + (static_cast <float> (rand() % 100) / 100.0f);
        fallingLeaves.push_back(l);
    }
}

void drawGround() {
    setMaterialColor(0.25f, 0.20f, 0.15f); 
    glBegin(GL_QUADS);
    glNormal3f(0.0f, 1.0f, 0.0f);
    glVertex3f(-1000.0f, 0.0f, -1000.0f);
    glVertex3f(1000.0f, 0.0f, -1000.0f);
    glVertex3f(1000.0f, 0.0f, 1000.0f);
    glVertex3f(-1000.0f, 0.0f, 1000.0f);
    glEnd();
}

void draw3DMan(float x, float y, float z, bool isShadow) {
    glPushMatrix();
    
    float bobbing = 0.0f;
    float moveSpeed = isSprinting ? 2.5f : 2.0f;

    if(isManMoving) {
        bobbing = 2.0f * fabs(sin(walkPhase * moveSpeed)); 
    }

    float crouchLower = 0.0f;
    if (isCrouching) {
        crouchLower = 12.0f; 
    }

    glTranslatef(x, y + bobbing - crouchLower, z);
    glRotatef(manRotationY, 0.0f, 1.0f, 0.0f); 

    auto setColor = [&](float r, float g, float b) {
        if (isShadow) glColor3f(0.0f, 0.0f, 0.0f);
        else setMaterialColor(r, g, b);
    };

    const float THIGH_LEN = 22.0f;
    const float CALF_LEN = 23.0f;
    const float TORSO_LEN = 55.0f;
    const float NECK_LEN = 8.0f;
    const float HEAD_SIZE = 11.0f;
    const float SHOULDER_WIDTH = 18.0f; 
    
    float leftHipAngle = 0.0f, rightHipAngle = 0.0f;
    float leftKneeAngle = 0.0f, rightKneeAngle = 0.0f;

    if (isManMoving) {
        float swingAmp = isSprinting ? 50.0f : 30.0f;
        if (isCrouching) swingAmp = 15.0f; 

        leftHipAngle = swingAmp * sin(walkPhase);
        rightHipAngle = swingAmp * sin(walkPhase + M_PI);
        if (leftHipAngle > 0) leftKneeAngle = leftHipAngle * 2.0f;
        if (rightHipAngle > 0) rightKneeAngle = rightHipAngle * 2.0f;
    }

    float crouchBend = 0.0f;
    if (isCrouching) {
        crouchBend = 45.0f; 
        leftHipAngle -= crouchBend;
        rightHipAngle -= crouchBend;
        leftKneeAngle += (crouchBend * 2.0f); 
        rightKneeAngle += (crouchBend * 2.0f);
    }

    // Left Leg
    glPushMatrix();
    glTranslatef(7.0f, THIGH_LEN + CALF_LEN, 0.0f); 
    glRotatef(leftHipAngle, 1.0f, 0.0f, 0.0f); 
    
    setColor(0.4f, 0.6f, 0.9f); 
    glPushMatrix();
    glRotatef(90.0f, 1.0f, 0.0f, 0.0f); 
    drawCylinder(6.0f, 5.5f, THIGH_LEN);
    glPopMatrix();
    
    glTranslatef(0.0f, -THIGH_LEN, 0.0f); 
    glRotatef(leftKneeAngle, 1.0f, 0.0f, 0.0f); 
    glPushMatrix();
    glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
    drawCylinder(5.5f, 5.0f, CALF_LEN);
    glPopMatrix();
    
    setColor(0.1f, 0.1f, 0.1f);
    glTranslatef(0.0f, -CALF_LEN, 3.0f);
    glScalef(5.5f, 3.0f, 10.0f);
    glutSolidCube(1.0f);
    glPopMatrix(); 

    // Right Leg
    glPushMatrix();
    glTranslatef(-7.0f, THIGH_LEN + CALF_LEN, 0.0f); 
    glRotatef(rightHipAngle, 1.0f, 0.0f, 0.0f); 
    
    setColor(0.4f, 0.6f, 0.9f); 
    glPushMatrix();
    glRotatef(90.0f, 1.0f, 0.0f, 0.0f); 
    drawCylinder(6.0f, 5.5f, THIGH_LEN);
    glPopMatrix();
    
    glTranslatef(0.0f, -THIGH_LEN, 0.0f); 
    glRotatef(rightKneeAngle, 1.0f, 0.0f, 0.0f); 
    glPushMatrix();
    glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
    drawCylinder(5.5f, 5.0f, CALF_LEN);
    glPopMatrix();
    
    setColor(0.1f, 0.1f, 0.1f);
    glTranslatef(0.0f, -CALF_LEN, 3.0f);
    glScalef(5.5f, 3.0f, 10.0f);
    glutSolidCube(1.0f);
    glPopMatrix(); 

    // Waist
    glTranslatef(0.0f, THIGH_LEN + CALF_LEN, 0.0f);
    
    if (isCrouching) {
         glRotatef(15.0f, 1.0f, 0.0f, 0.0f);
    }

    // Belt
    glPushMatrix();
    glTranslatef(0.0f, 2.0f, 0.0f); 
    setColor(0.1f, 0.1f, 0.1f); 
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f); 
    drawCylinder(14.0f, 14.0f, 4.0f); 
    glPopMatrix();

    if(!isShadow) {
        setMaterialColor(0.8f, 0.8f, 0.8f); 
        glPushMatrix();
        glTranslatef(0.0f, 4.0f, 13.5f); 
        glScalef(5.0f, 4.0f, 1.5f);
        glutSolidCube(1.0f);
        glPopMatrix();
    }

    // Torso
    setColor(0.0f, 0.8f, 0.9f); 
    glPushMatrix();
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f); 
    
    glPushMatrix();
    glScalef(1.3f, 0.9f, 1.0f); 
    drawCylinder(13.0f, 15.0f, TORSO_LEN);
    
    glTranslatef(0.0f, 0.0f, TORSO_LEN);
    GLUquadricObj *disk = gluNewQuadric();
    gluDisk(disk, 0.0f, 15.0f, 16, 1);
    gluDeleteQuadric(disk);
    glPopMatrix();
    glPopMatrix(); 

    if (!isShadow) {
        setMaterialColor(1.0f, 1.0f, 1.0f); 
        glPushMatrix();
        glTranslatef(0.0f, TORSO_LEN * 0.6f, 12.5f);
        glScalef(22.0f, 4.0f, 1.0f);
        glutSolidCube(1.0f);
        glPopMatrix();
        setMaterialColor(1.0f, 0.8f, 0.0f); 
        glPushMatrix();
        glTranslatef(0.0f, TORSO_LEN * 0.6f, 13.5f); 
        glScalef(4.0f, 4.0f, 1.0f);
        glRotatef(45.0f, 0.0f, 0.0f, 1.0f); 
        glutSolidCube(1.0f);
        glPopMatrix();
    }

    // Shoulders
    glTranslatef(0.0f, TORSO_LEN, 0.0f); 

    float armSwing = 0.0f;
    if (isManMoving) {
        float armAmp = isSprinting ? 40.0f : 25.0f;
        armSwing = -armAmp * sin(walkPhase);
    }
    
    if (isCrouching) {
        armSwing *= 0.5f; 
        glRotatef(20.0f, 1.0f, 0.0f, 0.0f); 
    }

    glPushMatrix();
    glTranslatef(SHOULDER_WIDTH, -2.0f, 0.0f); 
    glRotatef(armSwing, 1.0f, 0.0f, 0.0f); 
    setColor(0.0f, 0.8f, 0.9f); 
    glPushMatrix();
    glRotatef(90.0f, 1.0f, 0.0f, 0.0f); 
    drawCylinder(4.0f, 3.0f, 40.0f);
    glPopMatrix();
    setColor(1.0f, 0.85f, 0.75f);
    glTranslatef(0.0f, -40.0f, 0.0f);
    glutSolidSphere(4.0f, 10, 10);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(-SHOULDER_WIDTH, -2.0f, 0.0f);
    glRotatef(-armSwing, 1.0f, 0.0f, 0.0f);
    setColor(0.0f, 0.8f, 0.9f);
    glPushMatrix();
    glRotatef(90.0f, 1.0f, 0.0f, 0.0f); 
    drawCylinder(4.0f, 3.0f, 40.0f);
    glPopMatrix();
    setColor(1.0f, 0.85f, 0.75f);
    glTranslatef(0.0f, -40.0f, 0.0f);
    glutSolidSphere(4.0f, 10, 10);
    glPopMatrix();

    // Neck
    setColor(0.0f, 0.5f, 0.6f);
    glPushMatrix();
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
    drawCylinder(6.0f, 6.0f, 3.0f);
    glPopMatrix();

    setColor(1.0f, 0.85f, 0.75f);
    glPushMatrix();
    glTranslatef(0.0f, 3.0f, 0.0f);
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
    drawCylinder(5.0f, 5.0f, NECK_LEN);
    glPopMatrix();

    glTranslatef(0.0f, NECK_LEN + 3.0f + (HEAD_SIZE/2.0f), 0.0f);
    
    if (isCrouching) {
         glRotatef(-20.0f, 1.0f, 0.0f, 0.0f);
    }

    // Head
    setColor(1.0f, 0.85f, 0.75f);
    glutSolidSphere(HEAD_SIZE, 16, 16);

    if (!isShadow) {
        // Hair
        setMaterialColor(0.2f, 0.1f, 0.0f);
        glPushMatrix();
        glTranslatef(0.0f, 4.0f, -1.0f);
        glScalef(1.05f, 0.8f, 1.05f);
        glutSolidSphere(HEAD_SIZE, 16, 16);
        glPopMatrix();

        // Headphones
        setMaterialColor(0.2f, 0.2f, 0.2f); 
        glPushMatrix();
        glTranslatef(0.0f, 1.0f, 0.0f); 
        glScalef(1.0f, 0.9f, 1.0f); 
        glutSolidTorus(1.0f, 12.0f, 8, 32); 
        glPopMatrix();
        
        setMaterialColor(0.1f, 0.1f, 0.1f);
        glPushMatrix();
        glTranslatef(11.0f, 0.0f, 0.0f);
        glScalef(1.0f, 3.0f, 2.5f);
        glutSolidSphere(2.5f, 10, 10);
        glPopMatrix();
        
        glPushMatrix();
        glTranslatef(-11.0f, 0.0f, 0.0f);
        glScalef(1.0f, 3.0f, 2.5f);
        glutSolidSphere(2.5f, 10, 10);
        glPopMatrix();

        // Face
        setMaterialColor(1.0f, 1.0f, 1.0f);
        glPushMatrix();
        glTranslatef(3.5f, 0.0f, 9.0f);
        glutSolidSphere(2.5f, 8, 8);
        setMaterialColor(0.0f, 0.0f, 0.0f);
        glTranslatef(0.0f, 0.0f, 2.1f);
        glutSolidSphere(1.0f, 8, 8);
        glPopMatrix();

        setMaterialColor(1.0f, 1.0f, 1.0f);
        glPushMatrix();
        glTranslatef(-3.5f, 0.0f, 9.0f);
        glutSolidSphere(2.5f, 8, 8);
        setMaterialColor(0.0f, 0.0f, 0.0f);
        glTranslatef(0.0f, 0.0f, 2.1f);
        glutSolidSphere(1.0f, 8, 8);
        glPopMatrix();

        setMaterialColor(1.0f, 0.8f, 0.7f);
        glPushMatrix();
        glTranslatef(0.0f, -2.0f, 10.0f);
        glutSolidCone(2.0f, 4.0f, 16, 16);
        glPopMatrix();

        setMaterialColor(0.7f, 0.3f, 0.3f);
        glPushMatrix();
        glTranslatef(0.0f, -6.0f, 9.5f);
        glRotatef(10.0f, 0.0f, 0.0f, 1.0f); 
        glScalef(3.0f, 0.8f, 1.0f);
        glutSolidSphere(1.0f, 10, 10);
        glPopMatrix();
    }

    glPopMatrix(); 
}

void draw3DTree(float x, float z) {
    glPushMatrix();
    glTranslatef(x, 0.0f, z);
    setMaterialColor(0.3f, 0.15f, 0.05f);
    glPushMatrix();
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
    drawCylinder(15.0f, 10.0f, 100.0f);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(0.0f, 100.0f, 0.0f);
    setMaterialColor(0.8f, 0.4f, 0.0f);
    glutSolidCone(50.0f, 60.0f, 16, 16);
    glTranslatef(0.0f, 30.0f, 0.0f);
    setMaterialColor(0.9f, 0.6f, 0.1f);
    glutSolidCone(50.0f, 60.0f, 16, 16);
    glPopMatrix();
    glPopMatrix();
}

void drawFallingLeaves() {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBegin(GL_QUADS);
    for (const auto& leaf : fallingLeaves) {
        setMaterialColor(leaf.color[0], leaf.color[1], leaf.color[2]);
        float currentX = leaf.x + 20.0f * sin(leafDriftSpeed + leaf.z * 0.1f);
        glVertex3f(currentX, leaf.y, leaf.z);
        glVertex3f(currentX + leaf.size, leaf.y, leaf.z);
        glVertex3f(currentX + leaf.size, leaf.y + leaf.size, leaf.z);
        glVertex3f(currentX, leaf.y + leaf.size, leaf.z);
    }
    glEnd();
    glDisable(GL_BLEND);
}

void drawShadow() {
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST); 
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glPushMatrix();
    GLfloat groundPlane[4] = {0.0f, 1.0f, 0.0f, 0.0f};
    GLfloat shadowMat[16];
    GLfloat dot = groundPlane[0] * lightPos[0] + groundPlane[1] * lightPos[1] + groundPlane[2] * lightPos[2] + groundPlane[3] * lightPos[3];
    shadowMat[0] = dot - lightPos[0] * groundPlane[0];
    shadowMat[4] = 0.0f - lightPos[0] * groundPlane[1];
    shadowMat[8] = 0.0f - lightPos[0] * groundPlane[2];
    shadowMat[12] = 0.0f - lightPos[0] * groundPlane[3];
    shadowMat[1] = 0.0f - lightPos[1] * groundPlane[0];
    shadowMat[5] = dot - lightPos[1] * groundPlane[1];
    shadowMat[9] = 0.0f - lightPos[1] * groundPlane[2];
    shadowMat[13] = 0.0f - lightPos[1] * groundPlane[3];
    shadowMat[2] = 0.0f - lightPos[2] * groundPlane[0];
    shadowMat[6] = 0.0f - lightPos[2] * groundPlane[1];
    shadowMat[10] = dot - lightPos[2] * groundPlane[2];
    shadowMat[14] = 0.0f - lightPos[2] * groundPlane[3];
    shadowMat[3] = 0.0f - lightPos[3] * groundPlane[0];
    shadowMat[7] = 0.0f - lightPos[3] * groundPlane[1];
    shadowMat[11] = 0.0f - lightPos[3] * groundPlane[2];
    shadowMat[15] = dot - lightPos[3] * groundPlane[3];
    glMultMatrixf(shadowMat);
    draw3DMan(manPositionX, 0.1f, manPositionZ, true);
    glPopMatrix();
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glDisable(GL_BLEND);
}

void initialize() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glClearColor(0.7f, 0.85f, 1.0f, 1.0f);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    GLfloat lightAmb[] = { 0.4f, 0.4f, 0.4f, 1.0f };
    GLfloat lightDiff[] = { 0.8f, 0.8f, 0.8f, 1.0f };
    glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmb);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiff);
    glEnable(GL_COLOR_MATERIAL);
    initializeLeaves();
}

void renderScene() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    float camX = distanceFromMan * sin(cameraAngle * M_PI / 180.0f);
    float camZ = distanceFromMan * cos(cameraAngle * M_PI / 180.0f);

    gluLookAt(camX, 150.0f, camZ, 
              0.0f, 60.0f, 0.0f,           
              0.0f, 1.0f, 0.0f);

    drawGround();
    draw3DTree(150.0f, -100.0f);
    draw3DTree(-150.0f, 50.0f);
    drawShadow();
    draw3DMan(manPositionX, 0.0f, manPositionZ, false);
    drawFallingLeaves();
    glutSwapBuffers();
}

void updateScene(int value) {
    for (auto& leaf : fallingLeaves) {
        leaf.y -= leaf.fallSpeed;
        if (leaf.y < 0) {
            leaf.y = 500.0f; leaf.x = (rand() % 600) - 300.0f; leaf.z = (rand() % 600) - 300.0f;
        }
    }
    leafDriftSpeed += 0.02f;

    float dx = 0.0f;
    float dz = 0.0f;
    float speed = 4.0f;
    if (isSprinting) speed = 8.0f;
    if (isCrouching) speed = 2.0f;

    if (keyStates['w']) dz -= speed; 
    if (keyStates['s']) dz += speed; 
    if (keyStates['a']) dx -= speed; 
    if (keyStates['d']) dx += speed; 

    if (dx != 0.0f || dz != 0.0f) {
        isManMoving = true;
        manPositionX += dx;
        manPositionZ += dz;
        manRotationY = atan2(dx, dz) * 180.0f / M_PI;
        
        float animSpeed = 0.2f;
        if (isSprinting) animSpeed = 0.4f;
        if (isCrouching) animSpeed = 0.1f;
        
        walkPhase += animSpeed; 
    } else {
        isManMoving = false;
        if (walkPhase > 0.0f) walkPhase = 0.0f; 
    }

    glutPostRedisplay();
    glutTimerFunc(16, updateScene, 0);
}

// Helper to normalize keys (Convert Ctrl+Codes and Uppercase to lowercase)
unsigned char normalizeKey(unsigned char key) {
    // Handle Ctrl+Key (ASCII 1-26)
    if (key >= 1 && key <= 26) return key + 96; 
    // Handle Uppercase
    if (key >= 'A' && key <= 'Z') return key + 32;
    return key;
}

void updateModifiers() {
    int mods = glutGetModifiers();
    isCrouching = (mods & GLUT_ACTIVE_SHIFT);
    isSprinting = (mods & GLUT_ACTIVE_CTRL);
}

void keyboardDown(unsigned char key, int x, int y) {
    key = normalizeKey(key);
    keyStates[key] = true; 
    if (key == 27) exit(0); 
    updateModifiers();
}

void keyboardUp(unsigned char key, int x, int y) {
    key = normalizeKey(key);
    keyStates[key] = false;
    updateModifiers();
}

void specialKeyInput(int key, int x, int y) {
    if (key == GLUT_KEY_UP) distanceFromMan = max(MIN_ZOOM, distanceFromMan - 10.0f);
    if (key == GLUT_KEY_DOWN) distanceFromMan = min(MAX_ZOOM, distanceFromMan + 10.0f);
    if (key == GLUT_KEY_LEFT) cameraAngle -= 5.0f;
    if (key == GLUT_KEY_RIGHT) cameraAngle += 5.0f;
    glutPostRedisplay();
}

void mouseInput(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON) { isRotating = (state == GLUT_DOWN); lastMouseX = x; } 
    else if (button == GLUT_RIGHT_BUTTON) { isZooming = (state == GLUT_DOWN); lastMouseY = y; }
}

void mouseMove(int x, int y) {
    if (isRotating) { cameraAngle -= (x - lastMouseX) * 0.5f; lastMouseX = x; }
    else if (isZooming) { distanceFromMan += (y - lastMouseY) * 2.0f; lastMouseY = y; }
    glutPostRedisplay();
}

void reshape(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, (GLfloat)w / (GLfloat)h, 1.0, 2000.0);
    glMatrixMode(GL_MODELVIEW);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    glutCreateWindow("Realistic Man - Crouch & Sprint");
    
    // Prevent OS key repeat from spamming events
    glutIgnoreKeyRepeat(1); 
    
    initialize();
    glutDisplayFunc(renderScene);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboardDown);
    glutKeyboardUpFunc(keyboardUp); 
    glutSpecialFunc(specialKeyInput);
    glutMouseFunc(mouseInput);
    glutMotionFunc(mouseMove);
    glutTimerFunc(16, updateScene, 0);
    glutMainLoop();
    return 0;
}
