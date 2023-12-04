#ifndef IMAGE_OPERATION_H_
#define IMAGE_OPERATION_H_

#include <string>
#include <vector>

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#ifdef _DEBUG
#pragma comment(lib, "opencv_world460d.lib")
#else
#pragma comment(lib, "opencv_world460.lib")
#endif

enum class ExportExtention { Jpg, Png };
enum class PageDirection { Dextrorsum, Sinistrorsum};
enum class ProcessOption {None, PageReversed, PageDevided };

class CImageOperation
{
public:
	CImageOperation(wchar_t* folder)
	{
		SetFolderPath(folder);
		GetFileNames();
	}
	~CImageOperation() {};
	void ExecuteThresholding();
	void TestSingleImage(unsigned int page);
	unsigned long long GetElementsQuantity();
	void TestAdaptively(unsigned int page);
	void ExecuteCutting();
	void TestCroppingWhiteSpace(unsigned int page);
	void ExecuteCropping();
	void ManuallyCrop();
	void SetPageDirection(unsigned int direction);
	void ExecuteCompressing();
	void ExecuteFixedTrimming();

	int m_threshold_value{};
	int threshold_kind{};
	int max_value{};
	/*順応方式関連*/
	int m_block_value{};
	int m_adaptive_constant{};
	int m_blur_value{};
	/*補整*/
	int m_sharpness{};
	double m_gamma{};
private:
	enum { GradationBase = 1, GradationMax = 256 };
	std::string m_folder_path;
	std::vector<std::string> m_file_names;
	void SetFolderPath(wchar_t* working_directory);
	bool GetFileNames();
	void FilterAllImages();
	void ExportImage(std::string file_name, cv::Mat& img, ExportExtention extention);
	void SharpenImage(cv::Mat &src, cv::Mat &dst);
	void AdjustBrightness(cv::Mat& src, cv::Mat& dst);
	void FilterImage(cv::Mat& src, cv::Mat& dst);
	std::string GetBaseFileName();
	void CutImagesVertically(const cv::Mat& img, std::vector<cv::Mat>& blocks, PageDirection page_direction);
	void CutImportedImages();
	PageDirection m_page_orientation = PageDirection::Dextrorsum;
	/*裁ち落とし*/
	void CroppingThread();
	void CropWhiteSpace(cv::Mat& src, cv::Mat& dst);
	std::vector<cv::Rect> FindBoundRectangle(cv::Mat& raw, std::vector<std::vector<cv::Point>> contours);
	cv::Rect CreateExtimusRectangle(std::vector<cv::Rect> bound_rect);
	void EliminateTrimmingArea(cv::Rect& rectangle);
	/*目視裁ち落とし*/
	cv::Rect SelectRegionOfInterest(cv::Mat& src);
	void ManuallyCroppingThread();
	/*他*/
	void CompressingThread();
	void ExecuteWholeProcess();
	void ExecuteRotation();

	void FitAgainstCentreLine(cv::Mat& src, cv::Mat& dst);
};

/*--------------------------------------------------------
 * CCITT T 6に未対応。下限深さが8ビットなので白黒出力は不可。
 * https://github.com/opencv/opencv/blob/master/modules/imgcodecs/src/grfmt_tiff.cpp
 * bool TiffEncoder::writeLibTiff( const std::vector<Mat>& img_vec, const std::vector<int>& params)
 * http://www.itu.int/rec/T-REC-T.6-198811-I/en
 *--------------------------------------------------------*/

#endif //IMAGE_OPERATION_H_
