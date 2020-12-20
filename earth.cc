#include "earth.h"
#include "config.h"

#include <vector>

// for M_PI constant
#define _USE_MATH_DEFINES
#include <math.h>
#include <stdlib.h>


Earth::Earth() {
    sphere_view = false;
}

Earth::~Earth() {
}

void Earth::Init(const std::vector<std::string> &search_path) {
    // init shader program
    shader_.Init();
    
    // init texture: you can change to a lower-res texture here if needed
    earth_tex_.InitFromFile(Platform::FindFile("earth-2k.png", search_path));

    // init geometry
    const int nslices = 10;
    const int nstacks = 10;

    std::vector<unsigned int> indices;
    std::vector<Point2> tex_coords;
    
    float x_tex = 0.0;
    float y_tex = 9.0;
    float y_bound = 90;
    float x_bound = 180;
    float y_interval = 180 / nstacks;
    float x_interval = 360 / nslices;
    
    for (float y = -y_bound; y < y_bound; y += y_interval) {
        x_tex = 9.0;
        for (float x = -x_bound; x <= x_bound; x += x_interval) {
            tex_coords.push_back(Point2(x_tex,y_tex));
            tex_coords.push_back(Point2(x_tex, y_tex - 0.1));

            Point3 p1 = LatLongToPlane(y, x);
            Point3 p2 = LatLongToPlane(y + y_interval, x);
            plane_vertices.push_back(p1);
            plane_vertices.push_back(p2);

            Point3 s1 = LatLongToSphere(y, x);
            Point3 s2 = LatLongToSphere(y + y_interval, x);
            sphere_vertices.push_back(s1);
            sphere_vertices.push_back(s2);
            
            Vector3 v1 = Vector3(0,0,1);
            plane_normals.push_back(v1);
            plane_normals.push_back(v1);            

            Vector3 v2 = s1 - Point3(0,0,0);
            Vector3 v3 = s2 - Point3(0,0,0);
            sphere_normals.push_back(v2);
            sphere_normals.push_back(v3);
            
            x_tex += 0.1;
            if (x > -x_bound) {  
                // one triangle
                indices.push_back(plane_vertices.size() - 4);  // 0
                indices.push_back(plane_vertices.size() - 2);  // 2
                indices.push_back(plane_vertices.size() - 3);  // 1
                
                // another triangle
                indices.push_back(plane_vertices.size() - 2);  // 2
                indices.push_back(plane_vertices.size() - 1);  // 3
                indices.push_back(plane_vertices.size() - 3);  // 1
            }
        }
        y_tex -= 0.1;
    }

    earth_mesh_.SetVertices(plane_vertices);
    earth_mesh_.SetIndices(indices);
    earth_mesh_.SetNormals(plane_normals);
    earth_mesh_.SetTexCoords(0, tex_coords);
    earth_mesh_.UpdateGPUMemory();
}



void Earth::Draw(const Matrix4 &model_matrix, const Matrix4 &view_matrix, const Matrix4 &proj_matrix) {
    // Define a really bright white light.  Lighting is a property of the "shader"
    DefaultShader::LightProperties light;
    light.position = Point3(10,10,10);
    light.ambient_intensity = Color(1,1,1);
    light.diffuse_intensity = Color(1,1,1);
    light.specular_intensity = Color(1,1,1);
    shader_.SetLight(0, light);

    // Adust the material properties, material is a property of the thing
    // (e.g., a mesh) that we draw with the shader.  The reflectance properties
    // affect the lighting.  The surface texture is the key for getting the
    // image of the earth to show up.
    DefaultShader::MaterialProperties mat;
    mat.ambient_reflectance = Color(0.5, 0.5, 0.5);
    mat.diffuse_reflectance = Color(0.75, 0.75, 0.75);
    mat.specular_reflectance = Color(0.75, 0.75, 0.75);
    mat.surface_texture = earth_tex_;

    // Draw the earth mesh using these settings
    if (earth_mesh_.num_triangles() > 0) {
        shader_.Draw(model_matrix, view_matrix, proj_matrix, &earth_mesh_, mat);
    }
}


Point3 Earth::LatLongToSphere(double latitude, double longitude) const {
    double x,y,z;
    latitude = GfxMath::ToRadians(latitude);
    longitude = GfxMath::ToRadians(longitude);
    x = cos(latitude) * sin(longitude);
    y = sin(latitude);
    z = cos(latitude) * cos(longitude);
    return Point3(x,y,z);
}


Point3 Earth::LatLongToPlane(double latitude, double longitude) const {
    double x, y;
    y = latitude * M_PI / 2 / 90;
    x = longitude * M_PI / 180;
    return Point3(x,y,0);
}



void Earth::DrawDebugInfo(const Matrix4 &model_matrix, const Matrix4 &view_matrix, const Matrix4 &proj_matrix) {
    // This draws a cylinder for each line segment on each edge of each triangle in your mesh.
    // So it will be very slow if you have a large mesh, but it's quite useful when you are
    // debugging your mesh code, especially if you start with a small mesh.
    for (int t=0; t<earth_mesh_.num_triangles(); t++) {
        std::vector<unsigned int> indices = earth_mesh_.triangle_vertices(t);
        std::vector<Point3> loop;
        loop.push_back(earth_mesh_.vertex(indices[0]));
        loop.push_back(earth_mesh_.vertex(indices[1]));
        loop.push_back(earth_mesh_.vertex(indices[2]));
        quick_shapes_.DrawLines(model_matrix, view_matrix, proj_matrix,
            Color(1,1,0), loop, QuickShapes::LinesType::LINE_LOOP, 0.005);
    }
}

