#include <sl/Camera.hpp>
#include "opencv2/opencv.hpp"
#include <stdio.h>

//using namespace sl;

//cv::Mat slMat2cvMat(Mat& input) {
//    // Mapping between MAT_TYPE and CV_TYPE
//    int cv_type = -1;
//    switch (input.getDataType()) {
//        case MAT_TYPE_32F_C1: cv_type = CV_32FC1; break;
//        case MAT_TYPE_32F_C2: cv_type = CV_32FC2; break;
//        case MAT_TYPE_32F_C3: cv_type = CV_32FC3; break;
//        case MAT_TYPE_32F_C4: cv_type = CV_32FC4; break;
//        case MAT_TYPE_8U_C1: cv_type = CV_8UC1; break;
//        case MAT_TYPE_8U_C2: cv_type = CV_8UC2; break;
 //       case MAT_TYPE_8U_C3: cv_type = CV_8UC3; break;
//        case MAT_TYPE_8U_C4: cv_type = CV_8UC4; break;
//        default: break;
//    }

    // Since cv::Mat data requires a uchar* pointer, we get the uchar1 pointer from sl::Mat (getPtr<T>())
    // cv::Mat and sl::Mat will share a single memory structure
//    return cv::Mat(input.getHeight(), input.getWidth(), cv_type, input.getPtr<sl::uchar1>(MEM_CPU));
//}

int main(int argc, char **argv) {
    int timer;
    sl::Camera zed;
    sl::Mesh mesh;
    sl::InitParameters init_params;
    sl::SpatialMappingParameters spatial_map;
    sl::TRACKING_STATE tracking_state;
    sl::TrackingParameters tracking_parameters;

    // Set configuration parameters
    init_params.camera_resolution = sl::RESOLUTION_HD720; // Use HD1080 video mode
    init_params.coordinate_system = sl::COORDINATE_SYSTEM_RIGHT_HANDED_Y_UP;
    init_params.depth_mode = sl::DEPTH_MODE_PERFORMANCE;
    init_params.coordinate_units = sl::UNIT_METER;

    // Open the camera
    sl::ERROR_CODE err = zed.open(init_params);
    if (err != sl::SUCCESS)
        exit(-1);

    // Capture 50 frames and stop
    // int i = 0;

    // tracking testsing
    //sl::TrackingParameters t_params;
    //zed.enableTracking(t_params);
    //sl::Pose zed_pose;
    ///
    
    //spatial mapping config
    //spatial_map.range_meter = sl::MAPPING_RANGE_FAT;
    //spatial_map.resolution_meter = sl::MAPPING_RESOLUTION_LOW;
    spatial_map.save_texture = false;
    spatial_map.max_memory_usage = 512;
    //spatial_map.use_chunk_only = USE_CHUNKS;

    sl::Mat image_zed(zed.getResolution(), sl::MAT_TYPE_8U_C4);
    //cv::Mat image_ocv = slMat2cvMat(image_zed);
        

    // sl::Mat depth_zed(zed.getResolution(), MAT_TYPE_32F_C1);
    sl::Mat depth_zed(zed.getResolution(), sl::MAT_TYPE_8U_C4);
    //cv::Mat depth_ocv= slMat2cvMat(depth_zed);

    // getting spatical mapping
    zed.enableTracking(tracking_parameters);
    zed.enableSpatialMapping(spatial_map);
	
	//cv::namedWindow("Hello, World!", 0);	
	//cv::namedWindow("Hello, World!", 1);	
     
     timer = 0;
     sl::SPATIAL_MAPPING_STATE state;
     while (timer < 30) {
        // Grab an image
     
        if (zed.grab() == sl::SUCCESS) {
            //state = zed.getSpatialMappingState();
            //if (state = SPATIAL_MAPPING_STATE_NOT_ENOUGH_MEMORY) {
            //    printf("oh no, no memory");
            //    break;
            //}
            printf("%i \n", timer);
            
            //TRACKING_STATE state = zed.getPosition(zed_pose, REFERENCE_FRAME_WORLD);
            // A new image is available if grab() returns SUCCESS
            //zed.retrieveImage(image_zed, VIEW_LEFT); // Get the left image
	    //zed.retrieveImage(depth_zed, VIEW_DEPTH);
	    //zed.retrieveMeasure(depth_zed, MEASURE_DEPTH);

	    //cv::imshow("Hello, World!", image_ocv);
	    //cv::imshow("Hello, World2!", depth_ocv);
            
            if (timer%30 == 0) {
                printf("mesh is ready to go\n");
                zed.requestMeshAsync();
            }
            printf("state: %s \n", sl::spatialMappingState2str(zed.getSpatialMappingState()).c_str());
            if (zed.getMeshRequestStatusAsync() == sl::SUCCESS && timer > 0) {
                printf("our mesh is building \n");
                zed.retrieveMeshAsync(mesh);
            }
	    int x = depth_zed.getWidth()/2,
		y = depth_zed.getHeight()/2;
	    float value;
	    depth_zed.getValue(x, y, &value);
	    //printf("%f\n", zed_pose.getTranslation().tx);
            printf("number of tri: %zu \n", mesh.getNumberOfTriangles()); 
             
            timer++;
	    if(cv::waitKey(10) == 27 ) break;
        } else {
            printf("shit");
        }
    }
    zed.disableSpatialMapping();
    zed.disableTracking();
    // Close the camera
    zed.close();
    return 0;
}
