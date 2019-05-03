#include "zedInit.h"
#include <sl/Camera.hpp>

using namespace sl;

void zedCameraInit(sl::Camera zed) {
    printf("zedCameraInit begin \n");
    InitParameters init_params;
    init_params.camera_resolution = RESOLUTION_HD720; // Use HD720 video mode (default fps: 60)
    init_params.coordinate_system = COORDINATE_SYSTEM_RIGHT_HANDED_Y_UP; // Use a right-handed Y-up coordinate system
    init_params.coordinate_units = UNIT_MILLIMETER; // Set units in meters
    init_params.depth_mode = DEPTH_MODE_PERFORMANCE;
    init_params.sdk_verbose = true;
    zed.open(init_params);
    printf("zedCameraInit end \n");
}

void zedMappingInit(sl::Camera zed) {
    printf("zedMappingInit begin \n");
    sl::SpatialMappingParameters mapping_parameters;
    mapping_parameters.set(SpatialMappingParameters::RESOLUTION_LOW);
    mapping_parameters.set(SpatialMappingParameters::RANGE_FAR); 
    mapping_parameters.save_texture = false;
    zed.enableSpatialMapping(mapping_parameters);
    printf("zedMappingInit end \n");
}

void zedTrackInit(sl::Camera zed) {
    printf("zedTrackingInit begin \n");
    sl::TrackingParameters tracking_parameters;
    //err = zed.enableTracking(tracking_parameters);
    zed.enableTracking(tracking_parameters);
    printf("zedTrackingInit end \n");
}
    
    
    
