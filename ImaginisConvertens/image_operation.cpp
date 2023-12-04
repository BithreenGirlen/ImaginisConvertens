
#include <Windows.h>

#include <limits>

#include "image_operation.h"
#include "character_conversion.h"

/*階層の取り込み*/
void CImageOperation::SetFolderPath(wchar_t *working_directory)
{
	std::wstring ws(working_directory);
	m_folder_path = utf8_encode(ws);
}
/*階層内のファイル名取得*/
bool CImageOperation::GetFileNames()
{
    WIN32_FIND_DATAA find_file_data;
    std::string file_path = m_folder_path + "\\*";

    HANDLE hFind = ::FindFirstFileA(file_path.c_str(), &find_file_data);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do 
        {
            if (!(find_file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) 
            {
                m_file_names.push_back(find_file_data.cFileName);
            }
        } while (::FindNextFileA(hFind, &find_file_data));

        ::FindClose(hFind);
        return true;
    }

    return false;
}
/*順応方式二値処理*/
void CImageOperation::FilterAllImages()
{
    std::string output_directory = m_folder_path + "\\Thresholded\\";
    ::CreateDirectoryA(output_directory.c_str(), nullptr);

    for (size_t i = 0; i < m_file_names.size(); ++i) {
        std::string input_path = m_folder_path + "\\" + m_file_names.at(i);
        cv::Mat input_img = cv::imread(input_path, cv::IMREAD_GRAYSCALE);
        if (input_img.empty()) return;
        cv::Mat blurred;
        cv::GaussianBlur(input_img, blurred, cv::Size(m_blur_value, m_blur_value), 0);
        cv::Mat output_img;
        cv::adaptiveThreshold(blurred, output_img, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY, m_block_value, m_adaptive_constant);
        ExportImage(output_directory + m_file_names.at(i).substr(0, m_file_names.at(i).find_last_of(".")) + ".jpg", output_img , ExportExtention::Png);
    }
}
/*二値処理用スレッドの実行*/
void CImageOperation::ExecuteThresholding()
{
    std::thread th([this]() { this->FilterAllImages(); });
    th.detach();
}
/*試行*/
void CImageOperation::TestSingleImage(unsigned int page)
{
    if (page > m_file_names.size() - 1) return;
    std::string input_path = m_folder_path + "\\" + m_file_names.at(page);
    cv::Mat input_img = cv::imread(input_path, cv::IMREAD_GRAYSCALE);
    if (input_img.empty()) return;
    cv::Mat output_img;
    FilterImage(input_img, output_img);
    cv::namedWindow("result", cv::WINDOW_AUTOSIZE | cv::WINDOW_KEEPRATIO);
    cv::imshow("result", output_img);
    cv::waitKey(0);
    cv::destroyWindow("result");
    ExportImage(m_folder_path + "\\" + m_file_names.at(page).substr(0, m_file_names.at(page).find_last_of(".")) + "_thresholded.jpg", output_img, ExportExtention::Png);
}
/*要素数の取得*/
unsigned long long CImageOperation::GetElementsQuantity()
{
    return m_file_names.size();
}
/*順応方式での試行*/
void CImageOperation::TestAdaptively(unsigned int page)
{
    if (page > m_file_names.size() - 1) return;
    std::string input_path = m_folder_path + "\\" + m_file_names.at(page);
    cv::Mat input_img = cv::imread(input_path, cv::IMREAD_GRAYSCALE);
    if (input_img.empty()) return;
    cv::Mat blurred;
    cv::GaussianBlur(input_img, blurred, cv::Size(m_blur_value, m_blur_value), 0);
    cv::Mat output_img;
    cv::adaptiveThreshold(blurred, output_img, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY, m_block_value, m_adaptive_constant);
    cv::namedWindow("adaptively", cv::WINDOW_AUTOSIZE | cv::WINDOW_KEEPRATIO);
    cv::imshow("adaptively", output_img);
    cv::waitKey(0);
    cv::destroyWindow("adaptively");
    ExportImage(m_folder_path + "\\" + m_file_names.at(page).substr(0, m_file_names.at(page).find_last_of(".")) + "_adapted.jpg", output_img, ExportExtention::Png);
}
/*鮮鋭化*/
void CImageOperation::SharpenImage(cv::Mat &src, cv::Mat &dst)
{
    cv::Mat blur;
    cv::GaussianBlur(src, blur, cv::Size(13, 13), 0);
    cv::addWeighted(src, 1.0 + m_sharpness, blur, -m_sharpness, 0, dst);
}
/*明暗度補正*/
void CImageOperation::AdjustBrightness(cv::Mat& src, cv::Mat& dst)
{
    unsigned char grad[GradationMax]{};
    double div = static_cast<double>(GradationMax) - 1.0;
    for (int i = 0; i < GradationMax; ++i) {
        grad[i] = static_cast<unsigned char>(pow(i / div, static_cast<double>(GradationBase) / m_gamma) * div);
    }
    cv::LUT(src, cv::Mat(GradationBase, GradationMax, CV_8U, grad),  dst);
}
/*閾値処理*/
void CImageOperation::FilterImage(cv::Mat& src, cv::Mat& dst)
{
    cv::Mat sharpened;
    cv::Mat brightned;
    SharpenImage(src, sharpened);
    AdjustBrightness(sharpened, brightned);
    cv::Mat img1, img2;
    cv::threshold(brightned, img1, m_threshold_value, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);
    cv::threshold(brightned, img2, m_threshold_value, 255, cv::THRESH_TOZERO_INV | cv::THRESH_OTSU);
    cv::bitwise_xor(img1, img2, dst);
}
/*基本ファイル名の選出*/
std::string CImageOperation::GetBaseFileName()
{
    std::string::size_type size = m_file_names.size();
    std::string first = m_file_names.at(0);
    std::string last = m_file_names.at(size-1);

    size_t length = (first.length() > last.length() ? last.length() : first.length());
    size_t pos;
    for (pos = 0; pos < length; ++pos) {
        if(first[pos] != last[pos])break;
    }
    if (pos != length)return first.substr(0, pos);
    else return m_folder_path.substr(m_folder_path.find_last_of("/\\") + 1);
}
/*見開き分割*/
void CImageOperation::CutImagesVertically(const cv::Mat& img,  std::vector<cv::Mat>& blocks, PageDirection page_direction)
{
    int img_width = img.cols;
    int img_height = img.rows;
    if (page_direction == PageDirection::Sinistrorsum) {
        blocks.push_back(img(cv::Rect(0, 0, img_width / 2, img_height)).clone());
        blocks.push_back(img(cv::Rect(img_width / 2, 0, img_width / 2, img_height)).clone());
    }
    else {
        blocks.push_back(img(cv::Rect(img_width / 2, 0, img_width / 2, img_height)).clone());
        blocks.push_back(img(cv::Rect(0, 0, img_width / 2, img_height)).clone());
    }
}
void CImageOperation::ExecuteCutting()
{
    std::thread th([this]() { this->CutImportedImages(); });
    th.detach();
}
void CImageOperation::CutImportedImages()
{
    std::string output_directory = m_folder_path + "\\Cut\\";
    ::CreateDirectoryA(output_directory.c_str(), nullptr);

    //for (size_t i = 0; i < m_file_names.size(); ++i) {
    //    std::string input_path = m_folder_path + "\\" + m_file_names.at(i);
    //    cv::Mat input_img = cv::imread(input_path, cv::IMREAD_UNCHANGED);
    //    if (input_img.empty()) return;
    //    std::vector<cv::Mat> blocks;
    //    CutImagesVertically(input_img, blocks, m_page_orientation);
    //    for (size_t j = 0; j < blocks.size(); ++j) {
    //        std::string page = std::to_string(2 * i + j + 1);
    //        std::string output_file_name = output_directory + page.insert(0, m_file_names.at(i).substr(0, m_file_names.at(i).find_last_of(".")).length() - page.length(), '0') + +".jpg";
    //        ExportImage(output_file_name, blocks.at(j), ExportExtention::Png);
    //    }
    //}


    for (size_t i = 0; i < m_file_names.size(); ++i) {
        std::string input_path = m_folder_path + "\\" + m_file_names.at(i);
        cv::Mat input_img = cv::imread(input_path, cv::IMREAD_GRAYSCALE);
        if (input_img.empty()) return;
        cv::Mat img1;
        FilterImage(input_img, img1);
        cv::Mat img2;
        //cv::threshold(img1, img2, m_threshold_value, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);
        cv::threshold(img1, img2, m_threshold_value, 255, cv::THRESH_BINARY);
        std::vector<cv::Mat> blocks;
        CutImagesVertically(img2, blocks, m_page_orientation);
        for (size_t j = 0; j < blocks.size(); ++j) {
            std::string page = std::to_string(2 * i + j + 1);
            std::string output_file_name = output_directory + page.insert(0, m_file_names.at(i).substr(0, m_file_names.at(i).find_last_of(".")).length() - page.length(), '0') + ".jpg";
            ExportImage(output_file_name, blocks.at(j), ExportExtention::Png);
        }
    }
}
void CImageOperation::ExportImage(std::string file_name, cv::Mat& img, ExportExtention extention)
{
    std::vector<int> params(2);
    switch (extention) {
    case ExportExtention::Jpg:
        params[0] = cv::IMWRITE_JPEG_QUALITY;
        params[1] = 72;
        //params[2] = IMWRITE_JPEG_OPTIMIZE;
        //params[3] = 1;
        break;
    case ExportExtention::Png:
        params[0] = cv::IMWRITE_PNG_COMPRESSION;
        params[1] = 9;
        //params[2] = IMWRITE_PNG_STRATEGY;
        //params[3] = IMWRITE_PNG_STRATEGY_DEFAULT;
    }

    cv::imwrite(file_name, img, params);
}
/*裁ち落とし試行*/
void CImageOperation::TestCroppingWhiteSpace(unsigned int page)
{
    if (page > m_file_names.size() - 1) return;
    std::string input_path = m_folder_path + "\\" + m_file_names.at(page);
    cv::Mat input_img = cv::imread(input_path, cv::IMREAD_GRAYSCALE);
    if (input_img.empty()) return;
    std::vector<std::vector<cv::Point>> contours;
    cv::Mat filtered_img;
    FilterImage(input_img, filtered_img);
    filtered_img = ~filtered_img;
    cv::findContours(filtered_img, contours, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE);
    std::vector<cv::Rect> bound_rect = FindBoundRectangle(input_img, contours);
    //std::vector<cv::Rect> lined_rect;
    //std::vector<cv::Rect> bound_rect;
    //for (int i = 0; i < contours.size(); ++i) {
    //    lined_rect.push_back(cv::boundingRect(contours.at(i)));
    //}
    //for (int i = 0; i < lined_rect.size(); ++i) {
    //    bound_rect.push_back(lined_rect.at(i));
    //}
    //if (bound_rect.empty())return;
    cv::Mat output_img = cv::imread(input_path, cv::IMREAD_UNCHANGED);
    cv::Mat raw_img;
    output_img.copyTo(raw_img);
    drawContours(output_img, contours, -1, cv::Scalar(0, 255, 0), 2, cv::FILLED);
    for (size_t i = 0; i < bound_rect.size(); ++i) {
        cv::rectangle(output_img, bound_rect.at(i), cv::Scalar(0, 0, 255), 2, cv::FILLED);
    }
    cv::Rect extimus = CreateExtimusRectangle(bound_rect);

   cv::Mat cropped_img = raw_img(extimus);
   cv::Mat fit_img;
   FitAgainstCentreLine(cropped_img, fit_img);

    ExportImage(m_folder_path + "\\" + m_file_names.at(page).substr(0, m_file_names.at(page).find_last_of(".")) + "_cropped.jpg", fit_img, ExportExtention::Png);
    cv::namedWindow("raw", cv::WINDOW_NORMAL | cv::WINDOW_KEEPRATIO);
    cv::imshow("raw", output_img);
    cv::waitKey(0);
    cv::destroyWindow("raw");
}
/*全頁裁ち落とし*/
void CImageOperation::ExecuteCropping()
{
    std::thread th([this]() { this->CroppingThread(); });
    th.detach();
}
/*裁ち落とし実行スレッド*/
void CImageOperation::CroppingThread()
{
    std::string output_directory = m_folder_path + "\\Cropped\\";
    CreateDirectoryA(output_directory.c_str(), nullptr);

    for (size_t i = 0; i < m_file_names.size(); ++i) {
        std::string input_path = m_folder_path + "\\" + m_file_names.at(i);
        cv::Mat input_img = cv::imread(input_path, cv::IMREAD_UNCHANGED);
        if (input_img.empty()) break;
        cv::Mat output_img;
        CropWhiteSpace(input_img, output_img);
        cv::Mat fit_img;
        FitAgainstCentreLine(output_img, fit_img);
        ExportImage(output_directory + m_file_names.at(i).substr(0, m_file_names.at(i).find_last_of(".")) + ".jpg", fit_img, ExportExtention::Png);
    }
}
/*輪郭候補の絞り込み*/
std::vector<cv::Rect> CImageOperation::FindBoundRectangle(cv::Mat& raw, std::vector<std::vector<cv::Point>> contours)
{
    int minimum_size = 40;
    int minimum_ratio = 10;
    std::vector<cv::Rect> lined_rect;
    std::vector<cv::Rect> bound_rect;
    int width_cut = raw.size().width/ minimum_ratio;
    int height_cut = raw.size().height/ minimum_ratio;

    do {
        for (size_t i = 0; i < contours.size(); ++i) {
            if (contours.at(i).size() > minimum_size) {
                lined_rect.push_back(cv::boundingRect(contours.at(i)));
            }
        }
        for (size_t i = 0; i < lined_rect.size(); ++i) {
            if (lined_rect.at(i).height > height_cut && lined_rect.at(i).width > width_cut) {
                bound_rect.push_back(lined_rect.at(i));
            }
        }
        /*いい加減だが再探索*/
        if(minimum_size)--minimum_size;
        minimum_ratio += 10;
    } while (bound_rect.empty());

    return bound_rect;
}
/*最外方形の作成*/
cv::Rect CImageOperation::CreateExtimusRectangle(std::vector<cv::Rect> bound_rect)
{
    cv::Rect extimus_rectangle{};
    extimus_rectangle.x = std::numeric_limits<int>::max();
    extimus_rectangle.y = std::numeric_limits<int>::max();

    int dextimus = 0;
    int infimus = 0;
    for (size_t i = 0; i < bound_rect.size(); ++i) {
        /*cv::Rect自体の幅・高さは用いずに最内座標と最外座標から方形を作る。*/
        if ((bound_rect.at(i).x == 0) || bound_rect.at(i).y == 0)continue;
        int right = bound_rect.at(i).x + bound_rect.at(i).width;
        int bottom = bound_rect.at(i).y + bound_rect.at(i).height;
        if (right > dextimus) dextimus = right;
        if (bottom > infimus) infimus = bottom;
        if (bound_rect.at(i).x < extimus_rectangle.x)extimus_rectangle.x = bound_rect.at(i).x;
        if (bound_rect.at(i).y < extimus_rectangle.y)extimus_rectangle.y = bound_rect.at(i).y;
    }
    if (extimus_rectangle.x != std::numeric_limits<int>::max()) {
        extimus_rectangle.width = dextimus - extimus_rectangle.x;
        extimus_rectangle.height = infimus - extimus_rectangle.y;
    }

    /*端を含まぬ候補がない場合は面積で再探索*/
    if (extimus_rectangle.width == 0) {
        double largest_area = 0;
        size_t index = 0;
        for (size_t i = 0; i < bound_rect.size(); ++i) {
            double area = (double)bound_rect.at(i).width * (double)bound_rect.at(i).height;
            if (area > largest_area) {
                largest_area = area;
                index = i;
            }
        }
        extimus_rectangle = bound_rect.at(index);
    }

    EliminateTrimmingArea(extimus_rectangle);

    return extimus_rectangle;
}
/*天地左右を切り落とす*/
void CImageOperation::EliminateTrimmingArea(cv::Rect& rectangle)
{
    int cut_x = rectangle.width / 120;
    int cut_y = rectangle.height / 60;
    rectangle.x += cut_x;
    rectangle.y += cut_y;
    rectangle.width -= 2 * cut_x;
    rectangle.height -= 2 * cut_y;
}

/*裁ち落とし処理*/
void CImageOperation::CropWhiteSpace(cv::Mat& src, cv::Mat& dst)
{
    cv::Mat input_img;
    if (src.type() != CV_8U) {
        cv::cvtColor(src, input_img, cv::COLOR_BGR2GRAY);
    }
    else {
        input_img = src.clone();
    }
    cv::Mat filtered_img;
    FilterImage(input_img, filtered_img);
    filtered_img = ~filtered_img;
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(filtered_img, contours, cv::RETR_TREE, cv::CHAIN_APPROX_TC89_KCOS);
    std::vector<cv::Rect> bound_rect = FindBoundRectangle(src, contours);
    if (bound_rect.empty())return;
    cv::Rect extimus = CreateExtimusRectangle(bound_rect);
    dst = src(extimus);
}
/*画像の範囲選択*/
cv::Rect CImageOperation::SelectRegionOfInterest(cv::Mat& src)
{
    cv::Rect selected_rectangle;
    cv::namedWindow("selected", cv::WINDOW_NORMAL | cv::WINDOW_KEEPRATIO);
    do {
        selected_rectangle = cv::selectROI("selected", src);
    } while (selected_rectangle.empty());
    cv::destroyWindow("selected");
    return selected_rectangle;
}
void CImageOperation::ManuallyCrop()
{
    std::thread th([this]() { this->ManuallyCroppingThread(); });
    th.detach();
}
void CImageOperation::ManuallyCroppingThread()
{
    std::string output_directory = m_folder_path + "\\Selected\\";
    CreateDirectoryA(output_directory.c_str(), nullptr);

    cv::Rect selected_rectangle;
    for (size_t i = 0; i < m_file_names.size(); ++i) {
        std::string input_path = m_folder_path + "\\" + m_file_names.at(i);
        cv::Mat input_img = imread(input_path, cv::IMREAD_UNCHANGED);
        if (input_img.empty()) break;
        selected_rectangle = SelectRegionOfInterest(input_img);
        if (selected_rectangle.empty())break;
        cv::Mat output_img = input_img(selected_rectangle);
        ExportImage(output_directory + m_file_names.at(i).substr(0, m_file_names.at(i).find_last_of(".")) + ".jpg", output_img, ExportExtention::Png);
    }
}
/*綴じ方向の記憶*/
void CImageOperation::SetPageDirection(unsigned int direction)
{
    if(direction)m_page_orientation = PageDirection::Sinistrorsum;
    else m_page_orientation = PageDirection::Dextrorsum;
}

/*8bit深さに変換*/
void CImageOperation::ExecuteCompressing()
{
    //std::thread th([this]() { this->CompressingThread(); });
    std::thread th([this]() { this->ExecuteWholeProcess(); });
    th.detach();
}
void CImageOperation::CompressingThread()
{
    std::string output_directory = m_folder_path + "\\Compressed\\";
    CreateDirectoryA(output_directory.c_str(), nullptr);

    for (size_t i = 0; i < m_file_names.size(); ++i) {
        std::string input_path = m_folder_path + "\\" + m_file_names.at(i);
        cv::Mat input_img = cv::imread(input_path, cv::IMREAD_GRAYSCALE);
        if (input_img.empty()) break;
        cv::Mat output_img;
        cv::threshold(input_img, output_img, m_threshold_value, 255, cv::THRESH_BINARY);
        ExportImage(output_directory + m_file_names.at(i), output_img, ExportExtention::Png);
    }
}
/*入力画像裁ち落とし*/
void CImageOperation::ExecuteFixedTrimming()
{
    std::thread th([this]() { this->ExecuteRotation(); });
    th.detach();
}

void CImageOperation::ExecuteWholeProcess()
{
    std::string output_directory = m_folder_path + "\\Processed\\";
    ::CreateDirectoryA(output_directory.c_str(), nullptr);

    for (size_t i = 0; i < m_file_names.size(); ++i) {
        std::string input_path = m_folder_path + "\\" + m_file_names.at(i);
        cv::Mat input_img = cv::imread(input_path, cv::IMREAD_GRAYSCALE);
        if (input_img.empty()) return;
        cv::Mat img1;
        FilterImage(input_img, img1);
        cv::Mat img2;
        //cv::threshold(img1, img2, m_threshold_value, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);
        cv::threshold(img1, img2, m_threshold_value, 255, cv::THRESH_BINARY);

        ExportImage(output_directory + m_file_names.at(i).substr(0, m_file_names.at(i).find_last_of(".")) + ".jpg", img2, ExportExtention::Png);

        //size_t page = m_file_names.size() - i - 1;
        //ExportImage(output_directory + m_file_names.at(page).substr(0, m_file_names.at(i).find_last_of("\\.")) + ".jpg", img2, ExportExtention::Png);
        
        //std::vector<cv::Mat> blocks;
        //CutImagesVertically(img2, blocks, m_page_orientation);
        //for (size_t j = 0; j < blocks.size(); ++j) {
        //    std::string page = std::to_string(2 * i + j + 1);
        //    std::string output_file_name = output_directory + page.insert(0, m_file_names.at(i).substr(0, m_file_names.at(i).find_last_of(".")).length() - page.length(), '0') +".jpg";
        //    ExportImage(output_file_name, blocks.at(j), ExportExtention::Png);
        //}
    }
}
/*画像回転*/
void CImageOperation::ExecuteRotation()
{
    std::string output_directory = m_folder_path + "\\Rotated\\";
    CreateDirectoryA(output_directory.c_str(), nullptr);
    for (size_t i = 0; i < m_file_names.size(); ++i) {
        std::string input_path = m_folder_path + "\\" + m_file_names.at(i);
        cv::Mat input_img = cv::imread(input_path, cv::IMREAD_UNCHANGED);
        if (input_img.empty()) return;
        cv::Mat output_img;
        if (i % 2) {
            cv::rotate(input_img, output_img, cv::RotateFlags::ROTATE_90_COUNTERCLOCKWISE);
        }
        else {
            cv::rotate(input_img, output_img, cv::RotateFlags::ROTATE_90_CLOCKWISE);
        }
        ExportImage(output_directory + m_file_names.at(i), output_img, ExportExtention::Png);

    }

}
/*喉の検出と幅調整*/
void CImageOperation::FitAgainstCentreLine(cv::Mat& src, cv::Mat& dst)
{
    cv::Mat input_img;
    if (src.type() != CV_8U) 
    {
        cv::cvtColor(src, input_img, cv::COLOR_BGR2GRAY);
    }
    else 
    {
        input_img = src.clone();
    }
    cv::Mat filtered_img;
    FilterImage(input_img, filtered_img);
    cv::Mat inverted_img;
    cv::bitwise_not(filtered_img,inverted_img);
    std::vector<cv::Vec4f> lines;
    cv::HoughLinesP(inverted_img, lines, static_cast<double>(src.rows * 9 / 10), CV_PI, m_threshold_value, static_cast<double>(src.rows * 9 / 10), static_cast<double>(src.rows * 9/10));
    double centre = std::numeric_limits<double>::max();
    size_t index = 0;
    for (size_t i = 0; i < lines.size(); ++i)
    {
        cv::Vec4i line = lines.at(i);
        if (pow(src.cols / 2 - line[0], 2) < pow(src.cols / 5, 2))
        {
            double distance = pow(src.cols / 2 - line[0], 2) + pow(src.rows - (line[1] - line[3]), 2);
            if (distance < centre)
            {
                centre = distance;
                index = i;
            }
        }

        //cv::line(src, cv::Point(line[0], line[1]), cv::Point(line[2], line[3]), cv::Scalar(0, 0, 255), 1);
    }
    cv::Rect rect(0, 0, input_img.cols, input_img.rows);
    if (centre < std::numeric_limits<double>::max())
    {
        cv::Vec4i line = lines.at(index);
        double cut = line[0] * 2 - static_cast<double>(src.cols);
        if (cut > 0)
        {
            rect.x += static_cast<int>(cut);
            rect.width -= static_cast<int>(cut);
        }
        else
        {
            /*cut < 0*/
            rect.width += static_cast<int>(cut) * 2;
        }
    }
    /*候補なしの場合は生データを格納*/
    dst = src(rect);
    //cv::namedWindow("lined", cv::WINDOW_NORMAL | cv::WINDOW_KEEPRATIO);
    //cv::imshow("lined", src);
    //cv::waitKey(0);
    //cv::destroyWindow("lined");

}
