#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/opencv.hpp"
#include <chrono>
using namespace cv;
using namespace std;
using namespace std::chrono;

constexpr double PI = 3.14159265358979323846;

std::vector<int> findIntersections(std::vector<int*> rightLines, std::vector<int*> leftLines) {
	// Find intersections of pairs of lines
	int* rightLine;
	int* leftLine;
	int x1L;
	int y1L;
	int x2L;
	int y2L;
	int x1R;
	int y1R;
	int x2R;
	int y2R;
	std::vector<int> xIntersectList;
	int xIntersectValue;
	for (std::vector<int*>::iterator rightIt = rightLines.begin(); rightIt != rightLines.end(); rightIt++) {
		for (std::vector<int*>::iterator leftIt = leftLines.begin(); leftIt != leftLines.end(); leftIt++) {
			// Get lines as 1x4 Mat
			rightLine = *rightIt;
			leftLine = *leftIt;

			// Get each point from both matrices
			x1L = leftLine[0];
			y1L = leftLine[1];
			x2L = leftLine[2];
			y2L = leftLine[3];
			x1R = rightLine[0];
			y1R = rightLine[1];
			x2R = rightLine[2];
			y2R = rightLine[3];

			// Find the x-coordinate where the lines intersect and save it in a list
			// Get slope of both lines
			int leftSlope = (y1L - y2L) / (x1L - x2L);
			int rightSlope = (y1R - y2R) / (x1R - x2R);

			// Get y-intercept of both lines
			int leftInterceptY = y1L - (leftSlope * x1L);
			int rightInterceptY = y1R - (rightSlope * x1R);

			if (rightSlope != leftSlope) {
				xIntersectValue = (rightInterceptY - leftInterceptY) / (leftSlope - rightSlope);
				xIntersectList.push_back(xIntersectValue);
			}
		}
	}
	return xIntersectList;
}

string type2str(int type);
int main(int argc, char** argv)
{
	VideoCapture cap("");  // Insert path to captured video here

	if (!cap.isOpened()) {
		std::cout << "Error opening vid" << endl;
		return -1;
	}

	Mat frame;
	cap >> frame;  // Get 1st frame


	while (!frame.empty()) {
		// Process frame

		// Create a mask by filtering image colors
		Mat hsvFrame;
		cv::cvtColor(frame, hsvFrame, cv::COLOR_BGR2HSV);  // Convert to HSV for easier colour filtering
		Mat greenMask;
		int lowerGreenBound[3] = { 25, 61, 70 };
		std::vector<int> lgb(lowerGreenBound, lowerGreenBound + 3);
		int upperGreenBound[3] = { 55, 190, 177 };
		std:vector<int> ugb(upperGreenBound, upperGreenBound + 3);
		cv::inRange(hsvFrame, lgb, ugb, greenMask);  // Image filter for only shades of green to pass
		cv::Size blurSize(5, 5);
		Mat gaussGreenMask;
		cv::GaussianBlur(greenMask, gaussGreenMask, blurSize, 2.0);  // Add Gaussian blur
		
		// Dilate the mask
		// This helps to fill in gaps within a single row
		// Also helps blend rows that are far from the camera together
		// Yields cleaner edges from Canny edge detection
		Mat dilatedMask;
		Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(5, 5));
		cv::dilate(gaussGreenMask, dilatedMask, kernel, cv::Point(-1, -1), 4);

		// Canny Edge Detection
		Mat edges;
		cv::Canny(dilatedMask, edges, 100, 200);
		
		// Hough Lines Probabilistic Transform
		Mat lines;
		cv::HoughLinesP(edges, lines, 1.0, PI / 180.0, 30, 25, 10);
		
		// Convert OpenCV Matrix to C++ 2D Array
		int **linesArray = new int*[lines.rows];
		for (int i = 0; i < lines.rows; i++) {
			linesArray[i] = lines.ptr<int>(i);
		}

		// Find left and right lines
		int x1;
		int y1;
		int x2;
		int y2;
		int slope;
		std::vector<int*> rightLines;
		std::vector<int*> leftLines;
		for (int i = 0; i < lines.rows; i++) {
			x1 = linesArray[i][0];
			y1 = linesArray[i][1];
			x2 = linesArray[i][2];
			y2 = linesArray[i][3];


			if (x1 != x2) {  // Skip if line is vertical
				slope = (y2 - y1) / (x2 - x1);

				if (slope > 1) {
					rightLines.push_back(linesArray[i]);
				}
				else if (slope < -1) {
					leftLines.push_back(linesArray[i]);
				}
			}
		}

		std::vector<int> xIntersectList = findIntersections(rightLines, leftLines);

		// Find the average x coordinate among all of the intersection points
		int avgXIntersect = 0;
		for (std::vector<int>::iterator xIt = xIntersectList.begin(); xIt != xIntersectList.end(); xIt++) {
			avgXIntersect += *xIt;
		}
		if (xIntersectList.size() != 0) {
			avgXIntersect /= xIntersectList.size();
		}
		else {
			std::cout << "No intersections on frame" << endl;
		}

		// Draw vanishing point on a copy of the frame
		Mat frameCopy = frame.clone();
		cv::circle(frameCopy, Point(avgXIntersect, frame.cols/2), 8, Scalar(0,0,255), -1);

		cout << "Frame" << endl;
		cv::imshow("Frame", frame);
		cv::imshow("Circle", frameCopy);

		char c = (char)waitKey(99999999999);
		if (c == 27) break;

		// Grab next frame
		cap >> frame;
	}

	cap.release();
	cv::destroyAllWindows();

	return 0;
}