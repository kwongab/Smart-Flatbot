#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>

#define DEG_PER_PIXEL		0.036

using namespace std;
using namespace cv;
#include "boost/asio.hpp"
#include <boost/filesystem.hpp>
#include <boost/system/error_code.hpp>
using namespace boost::asio;

Scalar light_green = Scalar(255, 255, 100);
Scalar yellow = Scalar(255, 255, 0);
Scalar red = Scalar(255, 0, 0);
bool UI = false;

bool isCircle(float height, float width);

int main(int argc, char* argv[]) {
	VideoCapture cap(0); // open the default camera

	Mat imgOriginal;
	if (!cap.isOpened()) {  // check if we succeeded
		cout << "cannot open camera" << endl;
		return -1;
	}
	cout << "Run UI windows? press lowercase 'y' to run with UI, press any key to run without" << endl;
	string input;
	cin >> input;
	if (input == "y") {
		UI = true;
	}
	int counter = 0;

	//set up UDP transfer degree and distance of ball
	io_service io_service;
	ip::udp::socket socket(io_service);
	ip::udp::endpoint remote_endpoint;

	socket.open(ip::udp::v4());

	// Past endpoints that were sent. 
	// Current endpoint is to the computer connected to the beaglebone. Uses port 8000

	  //remote_endpoint = ip::udp::endpoint(ip::address::from_string("69.91.176.69"), 8000);
	  //remote_endpoint = ip::udp::endpoint(ip::address::from_string("192.168.0.51"), 9000);
	remote_endpoint = ip::udp::endpoint(ip::address::from_string("173.250.234.162"), 8000);
	boost::system::error_code err;

	while (true) {
		cap >> imgOriginal;
		Mat imgHSV, imgThreshSet, imgThreshSmooth, imgCanny, imgObj;
		vector<vector<Point> > contours;
		vector<Vec4i> hierarchy;
		cvtColor(imgOriginal, imgHSV, COLOR_BGR2HSV);

		// Set threshold values for detecting HSV color of a desired object, this case a tennis ball
			//alternative values
			//inRange(imgHSV, Scalar(18, 29, 10), Scalar(36, 168, 255), imgThreshSet);
			//inRange(imgHSV, Scalar(27, 32, 41), Scalar(83, 255, 255), imgThreshSet;	
		inRange(imgHSV, Scalar(27, 70, 45), Scalar(67, 255, 255), imgThreshSet);
		Mat imgThresh = imgThreshSet;

		// smooths out the image to get a better image 
		erode(imgThresh, imgThreshSmooth, getStructuringElement(MORPH_RECT, Size(3, 3)));
		dilate(imgThreshSmooth, imgThreshSmooth, getStructuringElement(MORPH_RECT, Size(3, 3)));

		// look on video feed for parts that match the HSV colors of a tennis ball, will find more than one
		GaussianBlur(imgThreshSmooth, imgThreshSmooth, Size(3, 3), 0, 0);
		Canny(imgThreshSmooth, imgCanny, 160.0, 80.0);
		findContours(imgCanny.clone(), contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE, Point(0, 0));
		vector<vector<Point> > hulls(contours.size());

		// get the first object that matches the threshold, this is usually the correct one
		int i = 0;
		approxPolyDP(contours[i], contours[i], 8.0, true);
		convexHull(Mat(contours[i]), hulls[i], CV_CLOCKWISE);
		imgObj = Mat::zeros(imgThresh.size(), CV_8UC3);

		// run only if there is HSV color that matches a tennis ball
		if (contours.size() > 0) {
			// draw a rectangle around the region matching the HSV values
			drawContours(imgObj, hulls, i, Scalar(255, 255, 255), 1, 8, hierarchy, 0, Point());
			drawContours(imgOriginal, hulls, i, Scalar(255, 255, 255), 1, 8, hierarchy, 0, Point());

			// Get rectangle and draw center
			Rect rectangle = boundingRect(hulls[i]);
			Point center = Point(rectangle.x + (int)((double)rectangle.width / 2.0), rectangle.y + (int)((double)rectangle.height / 2.0));

			// Get size of rectangle surrounding the ball
			float rectangleHeight = rectangle.height;
			float rectangleWidth = rectangle.width;
			float degrees = DEG_PER_PIXEL * (center.x - (imgOriginal.cols / 2));

			// Gets rid of noise value
			if (rectangleHeight > 1 && isCircle(rectangleHeight, rectangleWidth)) {
				cout << "(" << center.x << ", " << center.y << ")";
				circle(imgOriginal, center, 1, Scalar(255, 0, 0), 2);

				// Get the string for direction that will be sent through UDP
				string directionString;
				ostringstream directionStream;
				directionStream << degrees;
				directionString = directionStream.str();

				// Get the string for distance that will be sent through UDP
				string distanceString;
				ostringstream distanceStream;
				distanceStream << rectangleHeight;
				distanceString = distanceStream.str();

				// Send the distance and direction through.
				socket.send_to(buffer((distanceString + "," + directionString), 16), remote_endpoint, 0, err);

				// Print out to string to debug
				cout << "\trectangleHeight, degrees: " << rectangleHeight << ", " << degrees << endl;
				cout << "height: " << rectangle.height << endl;
			}
		}
		if (UI) {
			//show the UI
			imshow("ball", imgObj);
			imshow("original image", imgOriginal);
		}
		//stops loop if a key is pressed with UI open. 
		int key = cv::waitKey(5);
		key = (key == 255) ? -1 : key;
		if (key >= 0)
			break;
	}
	//close out of camera, or else pi will need to restart in order to run program
	socket.close();
	cap.release();
	return 0;
}

// checks to make sure the object is boxy and not a long thin strip 
bool isCircle(float height, float width) {
	if (height / width < 2.0 && height / width > 0.5) {
		return true;
	}
	return false;
}
