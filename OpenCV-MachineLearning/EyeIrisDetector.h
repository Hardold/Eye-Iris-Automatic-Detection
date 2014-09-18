#ifndef EyeIrisDetector_h
#define EyeIrisDetector_h

#include "opencv/cv.h"
#include "opencv2/objdetect/objdetect.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include <iostream>
#include <vector>
#include <queue>

#include "CascadeFeatureExtractor.h"

using namespace std;
using namespace cv;

class EyeIrisDetector
{
private:
	vector<Rect>faces;
	int colorFreq[256 + 5];
	#define WINDOWS_THRESHOLD 50

	struct Window
	{
		Rect rect;
		double score;
		int intensity_sum;
		Window()
		{}
		Window(Rect rect, double entropy_score, int intensity_sum) : rect(rect), score(entropy_score), intensity_sum(intensity_sum)
		{}
		bool operator < (const Window &w)const
		{
			return score<w.score;
		}
	};


public:

	EyeIrisDetector()
	{}

	vector< pair<Point2i,Point2i> >iris;

	void ExtractIris(Mat &img)
	{
		cvtColor(img, img, CV_BGR2GRAY);
		getFaces(img); //get face only in the image using viola and jones cascade classifier
		cropUpper();
		iris.clear();
		for(int i = 0 ; i<faces.size() ; i++)
		{
			vector<Window> bestWindows = getBestWindows(faces[i],img);
			double total_entropy=0;
			for(int j = 0 ; j<bestWindows.size() ; j++)
				total_entropy += bestWindows[j].score;

			vector<double> darkness_scores(bestWindows.size(),0);

			int total_darkness=0;
			for(int j = 0 ; j<bestWindows.size() ; j++)
			{
				darkness_scores[j] = calcIrisDarknessScore(bestWindows[j].rect,img);
				total_darkness += darkness_scores[j];
			}

			Window best_left,best_right;

			for(int j = 0 ; j<bestWindows.size() ; j++)
			{
				double Hscore = bestWindows[j].score/total_entropy;
				double Cscore = (double)darkness_scores[j]/(double)total_darkness;
				double Tscore = Hscore + Cscore;
				// left eye window
				if(bestWindows[j].rect.x < faces[i].width/2)
				{
					if(best_left.score<Tscore)
					{
						best_left.score=Tscore;
						best_left.rect=bestWindows[j].rect;
					}
				}
				// right eye window
				else
				{
					if(best_right.score<Tscore)
					{
						best_right.score=Tscore;
						best_right.rect=bestWindows[j].rect;
					}

				}
			}
			Point2i left_iris = Point2i(best_left.rect.x+(best_left.rect.width/2.0, best_left.rect.y+(best_left.rect.height/2.0)));
			Point2i right_iris = Point2i(best_right.rect.x+(best_right.rect.width/2.0, best_right.rect.y+(best_right.rect.height/2.0)));
			iris.push_back(make_pair(left_iris,right_iris));
		}	
	}



private:

	void getFaces(Mat &img)
	{
		CascadeFeatureExtractor faceCascade("D:\\Osama\\Programming\\OpenCV\\opencv\\sources\\data\\haarcascades\\haarcascade_frontalface_alt.xml");
		faceCascade.ExtractFaces(img);
		for(int i = 0 ; i<faceCascade.featureWindows.size() ;i++)
			faces.push_back(faceCascade.featureWindows[i]);
	}

	void cropUpper()
	{
		for(int i = 0 ; i<faces.size() ; i++)
			faces[i].height = faces[i].height/2;
	}

	vector<Window> getBestWindows(const Rect &face, const Mat &img)
	{
		Mat croppedFace = img(face);
		int d = face.width;
		double r = (double)d/6.0;
		const int deltaX = 3;
		const int deltaY = 1;
		const int windowWidth = 2*r + deltaX;
		const int windowHeight = 2*r + deltaY; 
		const int stepx = 5;
		const int stepy = 5;

		priority_queue<Window> bestWindowsPQ;

		for(int i = 0 ; i+windowHeight<croppedFace.rows ; i+=stepx)
			for(int j = 0 ; j+windowWidth<croppedFace.cols ; j+=stepy)
			{
				Rect rect = Rect(j,i,windowWidth,windowHeight);
				int intensity_sum=0;
				double score = calcEntropyScore(rect,croppedFace);
				bestWindowsPQ.push(Window(rect,score,intensity_sum));
				if(bestWindowsPQ.size()>WINDOWS_THRESHOLD)
					bestWindowsPQ.pop();
			}
			vector<Window> ret;
			while(!bestWindowsPQ.empty())
				ret.push_back(bestWindowsPQ.top()), bestWindowsPQ.pop();
			return ret;
	}

	double calcEntropyScore(const Rect & crop_window, const Mat &img)
	{
		memset(colorFreq,0,sizeof(colorFreq));
		Mat interestImg = img(crop_window);
		for(int i=0 ; i<interestImg.rows ; i++)
			for(int j=0 ; j<interestImg.cols ; j++)
				++colorFreq[interestImg.at<uchar>(i,j)];
		double entropyVal = 0;
		for(int i= 0 ; i<256 ; i++)
			entropyVal += calcEntropy(i,crop_window);
		return entropyVal;
	}

	double calcEntropy(int x, const Rect & crop_window)
	{
		double px = (double)colorFreq[x]/(double)(crop_window.width*crop_window.height);
		return - px * (log(px)/log(2.0));
	}

	int squareDistance(Point2i from, Point2i to)
	{
		return (from.x-to.x)*(from.x-to.x) + (from.y-to.y)*(from.y-to.y);
	}

	int calcIrisDarknessScore(const Rect &crop_window, const Mat &img)
	{
		int d = crop_window.width;
		double r = (double)d/6.0;
		int intensitySum=0;
		Point2i center(crop_window.x + crop_window.width/2.0 , crop_window.y + crop_window.height/2.0);

		for(int i = 0 ; i<crop_window.height ; i++)
			for(int j = 0 ; j<crop_window.width ; j++)
				if(squareDistance(center,Point2i(i,j))<= r*r)
					intensitySum += img.at<uchar>(i,j);
		return intensitySum;
	}

};

#endif