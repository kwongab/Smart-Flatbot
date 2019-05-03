#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include "opencv2/highgui.hpp"
#include <iostream>
#include <stdio.h>
#include "zedInit.h" 
#include <sl/Camera.hpp> 
//#include "RaspiCamCV.h"
// https://www.e-consystems.com/Articles/Camera/opencv-jetson-using-13MP-MIPI-camera.asp

#define CONE_HEIGHT			0.4953
#define CAM_CONSTANT		1583.5
#define DIST_CONSTANT		CONE_HEIGHT * CAM_CONSTANT
#define DEG_PER_PIXEL		0.036

using namespace std;
using namespace cv;
using namespace sl;
#include "boost/asio.hpp"
#include <boost/filesystem.hpp>
#include <boost/system/error_code.hpp>
using namespace boost::asio;

Scalar light_green = Scalar(255, 255, 100);
Scalar yellow = Scalar(255, 255, 0);
Scalar red = Scalar(255, 0, 0);
bool UI = false;
int view = 1;

bool isCircle(float height, float width);
float distanceBetween (float degreesBetweenBalls []);
cv::Mat slMat2cvMat(sl::Mat& input2);

int main(int argc, char* argv[]) {
  
	//VideoCapture cap(0); // open the default camera
  Camera zed;
  cout << "running the program here 0000000000" << endl;
	//zedCameraInit(zed);
    printf("zedCameraInit begin \n");
    InitParameters init_params;
    //init_params.camera_resolution = RESOLUTION_HD720; // Use HD720 video mode (default fps: 60)
    init_params.camera_resolution = RESOLUTION_VGA; // Use 480p video mode (default fps: 60)
    init_params.camera_fps = 15; //frames per second
    init_params.coordinate_system = COORDINATE_SYSTEM_RIGHT_HANDED_Y_UP; // Use a right-handed Y-up coordinate system
    init_params.coordinate_units = UNIT_MILLIMETER; // Set units in meters
    init_params.depth_mode = DEPTH_MODE_PERFORMANCE;
    init_params.sdk_verbose = true;
    zed.open(init_params);
    printf("zedCameraInit end \n");
	//zedMappingInit(zed);
    printf("zedTrackingInit begin \n");
    sl::TrackingParameters tracking_parameters;
    //err = zed.enableTracking(tracking_parameters);
    zed.enableTracking(tracking_parameters);
    printf("zedTrackingInit end \n");
    //zedTrackInit(zed);
    printf("zedMappingInit begin \n");
    sl::SpatialMappingParameters mapping_parameters;
    mapping_parameters.set(SpatialMappingParameters::RESOLUTION_LOW);
    mapping_parameters.set(SpatialMappingParameters::RANGE_FAR); 
    mapping_parameters.save_texture = false;
    //zed.enableSpatialMapping(mapping_parameters);
    printf("zedMappingInit end \n");
	/*zed.open(init_params);
	zed.enableTracking(track_params);
	zed.enableSpatialMapping(spat_params);*/
  cout << "running the program here111111111111 " << endl;
	sl::Mat imgOriginalSl;
  cv::Mat imgOriginal;
  /*
	if (!cap.isOpened()) {  // check if we succeeded
		cout << "cannot open camera" << endl;
		return -1;
	} */
	cout << "Run UI windows? press lowercase 'y' to run with UI, press any key to run without" << endl;
	string input;
	cin >> input;
	if (input == "y") {
		UI = true;
	}
   
  string input2; 
  /*
	do {
     cout << "How many ZED camera lenses to run. Enter either a 1 or 2" << endl;       
	    cin >> input2;
   } while ( input2 != "1" && input2 != "2");   
   std::istringstream ss(input2);
   ss >> view;
   cout << "running with " << view << " cameras" << endl;
   cout << "Exit program by pressing any key in window "<< endl;
   */ 
   view = 1;
	int counter = 0;
  
	//set up UDP transfer for degree and distance of ball
	io_service io_service;
	ip::udp::socket socket(io_service);
	ip::udp::endpoint remote_endpoint;

	socket.open(ip::udp::v4());
	// Past endpoints that were sent. 
	// Current endpoint is to the computer connected to the beaglebone. Uses port 8000

	  //remote_endpoint = ip::udp::endpoint(ip::address::from_string("69.91.176.69"), 8000);
	remote_endpoint = ip::udp::endpoint(ip::address::from_string("192.168.0.51"), 9000);
	//remote_endpoint = ip::udp::endpoint(ip::address::from_string("173.250.234.162"), 8000);
	boost::system::error_code err;
 
  // Create a sl::Mat with float type (32-bit)
  sl::Mat depth_zed(zed.getResolution(), MAT_TYPE_32F_C1);
  // Create an OpenCV Mat that shares sl::Mat data
  cv::Mat depth_ocv = slMat2cvMat(depth_zed); 
  bool grabbed = false;
	while (true) {
   grabbed = false;
   if (zed.grab() == SUCCESS) {
     grabbed = true; 
   }
		zed.retrieveImage(imgOriginalSl, VIEW_LEFT, MEM_CPU);
    imgOriginal = slMat2cvMat(imgOriginalSl);
		cv::Mat imgHSV, imgThreshLow, imgThreshHigh, imgThreshSmooth, imgCanny, imgObj;
		vector<vector<Point> > contours;
		vector<Vec4i> hierarchy;
		cv::cvtColor(imgOriginal, imgHSV, COLOR_BGR2HSV);

		// Set threshold values for detecting HSV color of a desired object, this case a tennis ball
			//alternative values
			//inRange(imgHSV, Scalar(18, 29, 10), Scalar(36, 168, 255), imgThreshLow);
			//inRange(imgHSV, Scalar(27, 32, 41), Scalar(83, 255, 255), imgThreshLow;	
		cv::inRange(imgHSV, Scalar(27, 70, 45), Scalar(67, 255, 255), imgThreshLow);
		cv::Mat imgThresh = imgThreshLow;

		// smooths out the image to get a better image 
		erode(imgThresh, imgThreshSmooth, getStructuringElement(MORPH_RECT, Size(3, 3)));
		dilate(imgThreshSmooth, imgThreshSmooth, getStructuringElement(MORPH_RECT, Size(3, 3)));

		// look on video feed for parts that match the HSV colors of a tennis ball, will find more than one
		GaussianBlur(imgThreshSmooth, imgThreshSmooth, Size(3, 3), 0, 0);
		Canny(imgThreshSmooth, imgCanny, 160.0, 80.0);
		findContours(imgCanny.clone(), contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE, Point(0, 0));
		vector<vector<Point> > hulls(contours.size());
    
		// gets all the area that have the correct HSV value
		for (int i = 0; i < contours.size(); i++) {
			approxPolyDP(contours[i], contours[i], 8.0, true);
			convexHull(cv::Mat(contours[i]), hulls[i], CV_CLOCKWISE);
		}
   
		imgObj = cv::Mat::zeros(imgThresh.size(), CV_8UC3);
    float rectangleHeight;
    float rectangleWidth;
    float degrees;
    string Result;
    string Result2;
    bool print = false;
		// get the first object for first two that matches the threshold, this is usually the correct ones
    float degreesBetweenBalls [2] = {0.0, 0.0};
    float positionBetweenBalls [2] = {0.0, 0.0};
		for (int i =0; i < view && i < contours.size(); i++) {
     
			// draw a rectangle around the region matching the HSV values
			if (UI) {
      
				drawContours(imgObj, hulls, i, Scalar(255, 255, 255), 1, 8, hierarchy, 0, Point());
				drawContours(imgOriginal, hulls, i, Scalar(255, 255, 255), 1, 8, hierarchy, 0, Point());
			}
			// get rectangle and draw center
			Rect rectangle = boundingRect(hulls[i]);
			Point center = Point(rectangle.x + (int)((double)rectangle.width / 2.0), rectangle.y + (int)((double)rectangle.height / 2.0));

			// Get size of rectangle surrounding the ball
			rectangleHeight = rectangle.height;
			rectangleWidth = rectangle.width;    
      //the ZED camera's width is 1344, which is what imgOriginal.cols gives 
      float lenseFactor = (imgOriginal.cols / 4);
      //checks if so checks if the ball is on right side
      if (center.x > (1344/2)){ 
        lenseFactor = (imgOriginal.cols / 4) * 3;
      }
			//degrees = DEG_PER_PIXEL * (center.x - lenseFactor); 
      degrees = DEG_PER_PIXEL * (center.x - imgOriginal.cols / 2); 
      degreesBetweenBalls[i] = degrees;
      positionBetweenBalls[i] = (center.x - lenseFactor);

			//gets rid of noise value
			if (rectangleHeight > 1 && isCircle(rectangleHeight, rectangleWidth)) {
				//cout << "(" << center.x << ", " << center.y << ")";
				circle(imgOriginal, center, 1, Scalar(255, 0, 0), 2);
        if (grabbed) {
            // Retrieve the depth measure (32-bit)
            zed.retrieveMeasure(depth_zed, MEASURE_DEPTH);
            // Print the depth value at the center of the image
            std::cout << "distance calculated from ZED: " << depth_ocv.at<float>(center.y, center.x) << std::endl;
        }				
				print = true;
			}
		}
     float between;
     float between2;
    if (print){
      between = distanceBetween(degreesBetweenBalls); 
      between2 = distanceBetween(positionBetweenBalls); 
      
      // Get the string for direction that will be sent through UDP
        degrees = degrees - between;
				ostringstream convert;
				convert << degrees;
				Result = convert.str();

				// Get the string for distance that will be sent through UDP
				ostringstream convert2;
				convert2 << rectangleHeight;
				Result2 = convert2.str();		
      
      // Send the distance and direction through.
      socket.send_to(buffer((Result2 + "," + Result), 16), remote_endpoint, 0, err);
      // Print out to string to debug
  		cout << "\trectangleHeight, degrees: " << rectangleHeight << ", " << degrees << endl;
    if (view == 2 && contours.size() > 1){      
      cout << "difference between two balls (degrees, distance): " << between << " , " << between2 << endl;
      
      }
  		cout << "height: " << rectangleHeight << endl;
   }
		if (UI) {
			//show the UI
			imshow("ball", imgObj);
			imshow("original image", imgOriginal);
		}
		else {
			imshow("dummy window to exit program", imgObj);
		}
		//stops loop if a key is pressed with UI open. 
		int key = cv::waitKey(5);
		key = (key == 255) ? -1 : key;
		if (key >= 0)
			break;
	}

	socket.close();
	//cap.release();
   //zed.disableSpatialMapping();
   zed.disableTracking();
   zed.close();
	cout << "successfuly closed camera" << endl;
}

// checks to make sure the object is boxy and not a long thin strip 
bool isCircle(float height, float width) {
	if (height / width < 2.0 && height / width > 0.5) {
		return true;
	}
	return false;
}

float distanceBetween (float degreesBetweenBalls []) {
  if (view == 2)
     return degreesBetweenBalls[1] - degreesBetweenBalls[0];   
  return 0;
}


cv::Mat slMat2cvMat(sl::Mat& input2) {
    // Mapping between MAT_TYPE and CV_TYPE
    int cv_type = -1;
    switch (input2.getDataType()) {
        case MAT_TYPE_32F_C1: cv_type = CV_32FC1; break;
        case MAT_TYPE_32F_C2: cv_type = CV_32FC2; break;
        case MAT_TYPE_32F_C3: cv_type = CV_32FC3; break;
        case MAT_TYPE_32F_C4: cv_type = CV_32FC4; break;
        case MAT_TYPE_8U_C1: cv_type = CV_8UC1; break;
        case MAT_TYPE_8U_C2: cv_type = CV_8UC2; break;
        case MAT_TYPE_8U_C3: cv_type = CV_8UC3; break;
        case MAT_TYPE_8U_C4: cv_type = CV_8UC4; break;
        default: break;
    }
    // Since cv::Mat data requires a uchar* pointer, we get the uchar1 pointer from sl::Mat (getPtr<T>())
    // cv::Mat and sl::Mat will share a single memory structure
    return cv::Mat(input2.getHeight(), input2.getWidth(), cv_type, input2.getPtr<sl::uchar1>(MEM_CPU));
}
