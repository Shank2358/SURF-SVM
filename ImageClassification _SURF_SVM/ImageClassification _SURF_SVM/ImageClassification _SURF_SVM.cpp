// ImageClassification _SURF_SVM.cpp: 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <iostream>  
#include <fstream>  
#include <sstream>  
#include <math.h>  
#include <opencv2/opencv.hpp>   
#include <opencv2/xfeatures2d.hpp> 
#include <opencv2/ml.hpp>

using namespace cv;
using namespace cv::xfeatures2d;
using namespace cv::ml;
using namespace std;

void read_csv(const string& filename, vector<Mat>& images, vector<int>& labels)
{
	char separator = ';';
	std::ifstream file(filename.c_str(), ifstream::in);
	if (!file) {
		string error_message = "No valid input file was given, please check the given filename.";
		CV_Error(CV_StsBadArg, error_message);
	}
	string line, path, classlabel;
	while (getline(file, line)) {
		stringstream liness(line);
		getline(liness, path, separator);
		getline(liness, classlabel);
		if (!path.empty() && !classlabel.empty()) {
			images.push_back(imread(path, 0));
			labels.push_back(atoi(classlabel.c_str()));
		}
	}
}

int main()
{
	//time
	clock_t start, end;

	//load datasets (images & labels)
	cout << "***** Load Images & Labels *****" << endl;
	//train datasets
	cout << "[Load training datasets]" << endl;
	vector<Mat> images_train;
	vector<int> labels_train;
	string fn_csv_train = "Data Set\\dataset1\\train\\at.txt";
	try
	{
		start = clock();
		read_csv(fn_csv_train, images_train, labels_train);
		end = clock();
	}
	catch (cv::Exception& e)
	{
		cerr << "Error opening file \"" << fn_csv_train << "\". Reason: " << e.msg << endl;
		exit(1);
	}
	if (images_train.size() <= 1) 
	{
		string error_message = "This demo needs at least 2 images to work. Please add more images to your dataset!";
		CV_Error(CV_StsError, error_message);
	}
	cout << "The number of training images: " << images_train.size() << endl;
	cout << "The run time of loading training datasets: " << double(end - start) / CLOCKS_PER_SEC << " seconds." << endl;

	//test datasets
	//train datasets
	cout << "[Load testing datasets]" << endl;
	vector<Mat> images_test;
	vector<int> labels_test;
	string fn_csv_test = "Data Set\\dataset1\\test\\at.txt";
	try
	{
		start = clock();
		read_csv(fn_csv_test, images_test, labels_test);
		end = clock();
	}
	catch (cv::Exception& e)
	{
		cerr << "Error opening file \"" << fn_csv_test << "\". Reason: " << e.msg << endl;
		exit(1);
	}
	if (images_test.size() < 1) {
		string error_message = "This demo needs at least 1 images to work. Please add more images to your dataset!";
		CV_Error(CV_StsError, error_message);
	}
	cout << "The number of testing images: " << images_test.size() << endl;
	cout << "The run time of loading testing datasets: " << double(end - start) / CLOCKS_PER_SEC << " seconds." << endl;

	//Extract Feature
	cout << endl;
	cout << "***** Extract Image Feature *****" << endl;
	vector<Mat> BagOfWords;
	int minHessian = 2000;
	int clusterNum = 100;
	start = clock();
	Mat descriptors;
	for (int i = 0; i < images_train.size(); i++)
	{
		vector<KeyPoint> keypoints;
		Ptr<SURF> detector = SURF::create(minHessian);
		detector->detect(images_train.at(i), keypoints);
		Mat descrip;
		Ptr<SURF> extractor = SURF::create();
		extractor->compute(images_train.at(i), keypoints, descrip);
		descriptors.push_back(descrip);
	}
	BOWKMeansTrainer* bowtrainer = new BOWKMeansTrainer(clusterNum);
	bowtrainer->add(descriptors);
	Mat dictionary = bowtrainer->cluster();
	Ptr<SURF> extractor = SURF::create();
	Ptr<DescriptorMatcher> matcher = DescriptorMatcher::create("BruteForce");
	BOWImgDescriptorExtractor* bowDE = new BOWImgDescriptorExtractor(extractor, matcher);
	bowDE->setVocabulary(dictionary);
	Mat bow;
	Mat labels;
	Ptr<SURF> featureDecter = SURF::create(minHessian);
	for (int i = 0; i < images_train.size(); i++)
	{
		vector<KeyPoint> kp;
		int lab = labels_train.at(i);
		Mat tem_image = images_train.at(i);
		Mat imageDescriptor;
		featureDecter->detect(tem_image, kp);
		bowDE->compute(tem_image, kp, imageDescriptor);
		normalize(imageDescriptor, imageDescriptor);
		bow.push_back(imageDescriptor);
		labels.push_back(lab);
	}
	end = clock();
	cout << "The run time of extracting image feature: " << double(end - start) / CLOCKS_PER_SEC << " seconds." << endl;

	//SVM training
	cout << endl;
	cout << "***** SVM Training *****" << endl;
	start = clock();
	Ptr<SVM> svm = SVM::create();
	svm->setType(SVM::C_SVC);
	svm->setKernel(SVM::RBF);
	svm->setGamma(0.01);
	svm->setC(10.0);
	//svm->setTermCriteria(TermCriteria(CV_TERMCRIT_EPS, 1000, FLT_EPSILON));
	svm->setTermCriteria(TermCriteria(TermCriteria::MAX_ITER, 1000, 1e-6));
	Ptr<TrainData> trainData = TrainData::create(bow, ROW_SAMPLE, labels);
	svm->train(trainData);
	svm->save("svmtrain.xml");
	end = clock();
	cout << "The run time of SVM training: " << double(end - start) / CLOCKS_PER_SEC << " seconds." << endl;

	//SVM testing
	cout << endl;
	cout << "***** Testing *****" << endl;
	vector<int> result;
	start = clock();
	for (int i = 0; i < images_test.size(); i++)
	{
		vector<KeyPoint>kp;
		Mat test;
		featureDecter->detect(images_test.at(i), kp);
		bowDE->compute(images_test.at(i), kp, test);
		normalize(test, test);
		result.push_back(svm->predict(test));
	}
	end = clock();
	cout << "The run time of testing: " << double(end - start) / CLOCKS_PER_SEC << " seconds." << endl;
	double correct = 0;
	double wrong = 0;
	for (int i = 0; i < result.size(); i++)
	{
		if (result.at(i) == labels_test.at(i))
		{
			correct++;
		}
		else
		{
			wrong++;
		}
	}
	cout << "The number of correct classifications: " << correct << endl;
	cout << "The number of wrong classifications: "<< wrong << endl;
	double proportion = correct / (correct + wrong);
	cout << "The proportion of correct classifications: " << proportion << endl;

	return 0;
}

