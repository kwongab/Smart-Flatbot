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

void cameraInit(sl::Camera &zed) {
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
        printf("camera failed to setup\n");
        exit(-1);
}

void trackSpatialInit(sl::Camera &zed) {
// Enable positional tracking with default parameters. Positional tracking needs to be enabled before using spatial mapping
    sl::TrackingParameters tracking_parameters;
    err = zed.enableTracking(tracking_parameters);
    if (err != SUCCESS)
        printf("tracking failed to setup\n");
        exit(-1);

    // Enable spatial mapping
    sl::SpatialMappingParameters mapping_parameters;
    mapping_parameters.set(SpatialMappingParameters::RESOLUTION_LOW);
    mapping_parameters.set(SpatialMappingParameters::RANGE_FAR); 
    mapping_parameters.save_texture = false;
    err = zed.enableSpatialMapping(mapping_parameters);
    if (err != SUCCESS)
        printf("spatial mapping failed to setup correctly\n");
        exit(-1);
}

int main(int argc, char **argv) {

    // Create a ZED camera object
    Camera zed;
    cameraInit(zed);
    trackSpatialInit(zed);

    int i;
    sl::Mesh mesh; // Create a mesh object
    sl::Mesh::chunkList visible_chunks;
    sl::Pose zed_pose;
    sl::TRACKING_STATE state;
    sl::ERROR_CODE err2;

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
                //printf("min distance: %f \n", zed.getDepthMinRangeValue());
                //printf("max distance: %f \n", zed.getDepthMaxRangeValue());
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
        }
        i++;
    }
    printf("the i: %i \n", i);
    zed.disableSpatialMapping();
    zed.disableTracking();
    zed.close();
    return 0;
}
