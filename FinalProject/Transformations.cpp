#include "Transformations.h"

// Linear Transformations

cv::Vec3b bilinearInterpolation(cv::Mat* source, double x, double y) {
	int y1 = static_cast<int>(floor(y));
	int y2 = std::min(static_cast<int>(ceil(y)), source->rows - 1);
	int x1 = static_cast<int>(floor(x));
	int x2 = std::min(static_cast<int>(ceil(x)), source->cols - 1);

	cv::Vec3b Q11 = source->at<cv::Vec3b>(y1, x1);
	cv::Vec3b Q12 = source->at<cv::Vec3b>(y1, x2);
	cv::Vec3b Q21 = source->at<cv::Vec3b>(y2, x1);
	cv::Vec3b Q22 = source->at<cv::Vec3b>(y2, x2);

	cv::Vec3b R1 = ((Q12 - Q11) / (x2 - x1)) * (x - x1) + Q11;
	cv::Vec3b R2 = ((Q22 - Q21) / (x2 - x1)) * (x - x1) + Q21;
	cv::Vec3b P = ((R2 - R1) / (y2 - y1)) * (y - y1) + R1;

	return P;
}

cv::Mat scale(cv::Mat* source, double sX, double sY, bool bilinearMethod) {
	cv::Mat output(static_cast<int>(source->rows * sY), static_cast<int>(source->cols * sX), CV_8UC3);

	if (bilinearMethod) {
		for (int j = 0; j < output.rows; j++)
		{
			double y = j / sY;

			for (int i = 0; i < output.cols; i++)
			{
				double x = i / sX;
				output.at<cv::Vec3b>(j, i) = bilinearInterpolation(source, x, y);
			}
		}

		return output;
	}

	for (int y = 0; y < output.rows; y++)
	{
		int v = static_cast<int>(y / sY);

		for (int x = 0; x < output.cols; x++)
		{
			int u = static_cast<int>(x / sX);
			output.at<cv::Vec3b>(y, x) = source->at<cv::Vec3b>(v, u);
		}
	}

	return output;
}

cv::Mat translation(cv::Mat* source, int tX, int tY) {
	cv::Mat output(source->rows, source->cols, CV_8UC3);

	for (int y = 0; y < output.rows; y++)
	{
		int v = y - tY;

		for (int x = 0; x < output.cols; x++)
		{
			int u = x - tX;
			if (v >= 0 && v < source->rows && u >= 0 && u < source->cols)
				output.at<cv::Vec3b>(y, x) = source->at<cv::Vec3b>(v, u);
			else
				output.at<cv::Vec3b>(y, x) = cv::Vec3b(0, 0, 0);
		}
	}

	return output;
}

cv::Mat rotation(cv::Mat* source, double r) {
	cv::Mat output(source->rows, source->cols, CV_8UC3);
	double theta = r * std::numbers::pi * 1 / 180;

	for (int y = 0; y < output.rows; y++)
	{
		double y0 = output.rows / 2.0 - y;

		for (int x = 0; x < output.cols; x++)
		{
			double x0 = x - output.cols / 2.0;

			double v0 = -x0 * sin(theta) + y0 * cos(theta);
			double u0 = x0 * cos(theta) + y0 * sin(theta);

			double v = output.rows / 2.0 - v0;
			double u = u0 + output.cols / 2.0;

			if (v >= 0 && v < source->rows && u >= 0 && u < source->cols)
			{
				cv::Vec3b p = bilinearInterpolation(source, u, v);
				output.at<cv::Vec3b>(y, x) = p;
			}
			else
				output.at<cv::Vec3b>(y, x) = cv::Vec3b(0, 0, 0);
		}
	}

	return output;
}

cv::Mat bias(cv::Mat* source, double bX, double bY)
{
	cv::Mat output(source->rows, source->cols, CV_8UC3);

	for (int y = 0; y < output.rows; y++)
	{
		double y0 = output.rows / 2.0 - y;

		for (int x = 0; x < output.cols; x++)
		{
			double x0 = x - output.cols / 2.0;

			double v0 = y0 - bY * x0;
			double u0 = x0 - bX * y0;

			double v = output.rows / 2.0 - v0;
			double u = u0 + output.cols / 2.0;

			if (v >= 0 && v < source->rows && u >= 0 && u < source->cols)
			{
				cv::Vec3b p = bilinearInterpolation(source, u, v);
				output.at<cv::Vec3b>(y, x) = p;
			}
			else
				output.at<cv::Vec3b>(y, x) = cv::Vec3b(0, 0, 0);
		}
	}

	return output;
}

cv::Mat allTransformations(cv::Mat* source, double sX, double sY, int tX, int tY, double r, double bX, double bY)
{
	cv::Mat output(static_cast<int>(source->rows * sY), static_cast<int>(source->cols * sX), CV_8UC3);
	cv::Mat mS = (cv::Mat_<double>(3, 3) <<
		sX, 0, 0,
		0, sY, 0,
		0, 0, 1);
	double theta = r * std::numbers::pi * 1 / 180;
	cv::Mat mR = (cv::Mat_<double>(3, 3) <<
		cos(theta), sin(theta), (1 - cos(theta)) * (output.cols * 0.5) - sin(theta) * (output.rows * 0.5),
		-sin(theta), cos(theta), sin(theta) * (output.cols * 0.5) + (1 - cos(theta)) * (output.rows * 0.5),
		0, 0, 1);
	cv::Mat mT = (cv::Mat_<double>(3, 3) <<
		1, 0, tX,
		0, 1, tY,
		0, 0, 1);
	cv::Mat mB = (cv::Mat_<double>(3, 3) <<
		1 + bX * bY, bX, 0,
		bY, 1, 0,
		0, 0, 1);

	cv::Mat transformations = mS * mR * mT * mB;

	for (int y = 0; y < output.rows; y++)
	{
		for (int x = 0; x < output.cols; x++)
		{
			cv::Mat V = (cv::Mat_<double>(3, 1) << x, y, 1);

			cv::Mat res = transformations * V;
			double w = res.at<double>(2, 0);
			double v = res.at<double>(1, 0);
			double u = res.at<double>(0, 0);

			if (v >= 0 && v < source->rows && u >= 0 && u < source->cols)
				output.at<cv::Vec3b>(y, x) = bilinearInterpolation(source, u, v);
			else
				output.at<cv::Vec3b>(y, x) = cv::Vec3b(0, 0, 0);
		}
	}

	return output;
}

// NonLinearTransformations