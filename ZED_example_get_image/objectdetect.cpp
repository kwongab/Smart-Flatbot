
#include <sl/Camera.hpp>  // version 2.0.1
#include <list>
#include <vector>
#include <cmath>

using namespace sl;

struct obstacles{
    float start;
    float end;
    int chunks_in;
    obstacles() : end(0), chunks_in(0)
    {
    }
};

bool chunk_Compare(sl::Chunk &chunkOne, sl::Chunk &chunkTwo) { 
    bool a;
    a = (chunkOne.barycenter[0] < chunkTwo.barycenter[0]);
    return a;
}

bool object_finder(sl::Chunk &object, std::vector<obstacles> &v, float dis) {
    if ((abs(object.barycenter[0] - v.back().end) > dis) /*&& (object.barycenter[2] > 0)*/) { 
        v.push_back(obstacles());
        v.back().start = v.back().end = object.barycenter[0];
        return true;
    }
    v.back().end = object.barycenter[0];
    v.back().chunks_in++;
    return false;
}

void obstacle_printer(std::vector<obstacles> v) {

    for (struct obstacles i : v) {
        printf("start: %f, end: %f, in: %i\n", i.start, i.end, i.chunks_in);
    }
}

// pauses spatial mapping 
bool pauseSpatialMapping(bool pause, Camera zed) {
    zed.pauseSpatialMapping(!pause);
    return pause;
}

int main(int argc, char **argv) {

    // Create a ZED camera object
    Camera zed;

    // Set configuration parameters
    InitParameters init_params;
    init_params.camera_resolution = RESOLUTION_HD720; // Use HD720 video mode (default fps: 60)
    init_params.coordinate_system = COORDINATE_SYSTEM_RIGHT_HANDED_Y_UP; // Use a right-handed Y-up coordinate system
    init_params.coordinate_units = UNIT_MILLIMETER; // Set units in meters
    init_params.depth_mode = DEPTH_MODE_PERFORMANCE;
    init_params.sdk_verbose = true;

    // Open the camera
    ERROR_CODE err = zed.open(init_params);
    if (err != SUCCESS)
        exit(-1);

    // Enable positional tracking with default parameters. Positional tracking needs to be enabled before using spatial mapping
    sl::TrackingParameters tracking_parameters;
    err = zed.enableTracking(tracking_parameters);
    if (err != SUCCESS)
        exit(-1);

    // Enable spatial mapping
    //MAPPING_RESOLUTION map_res = MAPPING_RESOLUTION_LOW;
    //MAPPING_RANGE map_range = MAPPING_RANGE_FAR;
    //sl::SpatialMappingParameters mapping_parameters(SpatialMappingParameters::MAPPING_RESOLUTION_LOW);
    //mapping_parameters.set(SpatialMappingParameters::MAPPING_RESOLUTION_LOW);
    //sl::SpatialMappingParameters mapping_parameters(SpatialMappingParameters::RESOLUTION::RES);
    sl::SpatialMappingParameters mapping_parameters;

    mapping_parameters.set(SpatialMappingParameters::RESOLUTION_HIGH);
    mapping_parameters.set(SpatialMappingParameters::RANGE_NEAR); 
    mapping_parameters.save_texture = false;
    err = zed.enableSpatialMapping(mapping_parameters);
    if (err != SUCCESS)
        exit(-1);

    // Grab data during 500 frames
    int i;
    sl::Mesh mesh; // Create a mesh object
    sl::Mesh::chunkList visible_chunks;
    sl::Pose zed_pose;
    sl::TRACKING_STATE state;
    sl::ERROR_CODE err2;
    //sl::Mesh::chunkList test_list:
    //int near_object = 0;
    while (err2 != ERROR_CODE_FAILURE) {
        // For each new grab, mesh data is updated 
        if (zed.grab() == SUCCESS) {
            std::vector<obstacles> v;
            state = zed.getPosition(zed_pose, sl::REFERENCE_FRAME_WORLD);
            // In the background, spatial mapping will use newly retrieved images, depth and pose to update the mesh
            sl::SPATIAL_MAPPING_STATE mapping_state = zed.getSpatialMappingState();
 
            if (i % 30 == 0) {
                zed.requestMeshAsync();
            }
            if (zed.getMeshRequestStatusAsync() == SUCCESS) {
                printf("mesh building \n");
                printf("min distance: %f \n", zed.getDepthMinRangeValue());
                printf("max distance: %f \n", zed.getDepthMaxRangeValue());
                err2 = zed.retrieveMeshAsync(mesh);
                visible_chunks = mesh.getSurroundingList(zed_pose.pose_data, 1700.0f);
                
                if (visible_chunks.size() > 0) {
                    std::sort (mesh.chunks.begin(), mesh.chunks.end(), chunk_Compare);
                    v.push_back(obstacles());
                    v[0].start = v[0].end = mesh.chunks[0].barycenter[0];
                    
                    for (sl::Chunk k : mesh.chunks) {
                        object_finder(k, v, 150.0f);
                    }
                
                    obstacle_printer(v);
                }

            }
            //printf("tri: %zu \n", mesh.getNumberOfTriangles());
            // Print spatial mapping state
            //std::cout << "\rImages captured: " << i << " / 500  ||  Spatial mapping state: " << spatialMappingState2str(mapping_state) << "                     " << std::flush;
        }
    }
    printf("the i: %i \n", i);
    //std::vector<obstacles> v;
    /*int objects = 0;
    float min_object_dis = 100.0;
    sl::Mesh::chunkList test_list;
    sl::Chunk test, test2, prev;

    // Extract, filter and save the mesh in a obj file
    //test = mesh[5];
    //test.vertices[0];
    //printf("Extracting Mesh...\n");
    zed.extractWholeMesh(mesh); // Extract the whole mesh
   //prev = mesh.chunks[0];
    test = mesh.chunks[0];
    std::sort (mesh.chunks.begin(), mesh.chunks.end(), chunk_Compare);
    
    v.push_back(obstacles);
    v[0].start = v[0].end = mesh.chunks[0].barycenter[0];

        printf("printing out chunks \n");
    for (sl::Chunk i : mesh.chunks) {
        printf("i: %f, test: %f\n", i.barycenter[0], test.barycenter[0]);
        if (i.barycenter[0] - test.barycenter[0] < 1000.0f) {
        //if (true) {
            near_object++;
        }
        //printf("xValues: %f \n", i.barycenter[0]);
        //printf("object at x: %f, y: %f, z: %f \n", i.barycenter[0], i.barycenter[1], i.barycenter[2]);
        //printf("distance: %f \n", sl::Vector3<float>::distance(i.barycenter, prev.barycenter));
        //prev = i;
        object_finder(i, v, min_object_dis); 
        
    }
    
    printf("printing vectors, of size %zu \n", v.size());
    for (struct obstacles i : v) {
        printf("start: %f, end: %f, in: %i \n", i.start, i.end, i. chunks_in);
    }
    printf("the entire list has : %zu, things\n", mesh.chunks.size());
    mesh.filter();
    mesh.save("test.obj");*/
    zed.disableSpatialMapping();
    zed.disableTracking();
    zed.close();
    return 0;
}
