# ImaginisConvertens
2021-03-01

書物資料の余白裁ち落とし・二色化(≠二値化)処理を行います。  
OpenCVでは深さ8ビットでの処理しかできないので、二値化(深さ1ビット)は別プログラムで処理します。→[Tiff](https://github.com/BithreenGirlen/Tiff)
## 要点説明
処理手順は次の通り。
1. 外枠領域の裁ち落とし
2. 閾値処理・明暗度補正・鮮鋭処理
3. 見開き裁断
4. 二色化(≠二値化)

この中で(1)の処理が最も長くなるため、説明は最後に回す。

## 閾値処理・明暗度補正・鮮鋭処理

### 閾値処理
閾値以下の値は白色に、閾値以上の値はそのまま、という処理を施す。
THRESH_BINARY と THRESH_TOZERO_INV のXOR演算を行えばよい。
``` cpp
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
```
### 鮮鋭処理
エッジを鮮明にする。
``` cpp
void CImageOperation::SharpenImage(cv::Mat &src, cv::Mat &dst)
{
    cv::Mat blur;
    cv::GaussianBlur(src, blur, cv::Size(13, 13), 0);
    cv::addWeighted(src, 1.0 + m_sharpness, blur, -m_sharpness, 0, dst);
}
```

### 明暗度補正
明度を調整する。
``` cpp
void CImageOperation::AdjustBrightness(cv::Mat& src, cv::Mat& dst)
{
    unsigned char grad[GradationMax]{};
    double div = static_cast<double>(GradationMax) - 1.0;
    int i;
    for (i = 0; i < GradationMax; ++i) {
        grad[i] = static_cast<unsigned char>(pow(i / div, static_cast<double>(GradationBase) / m_gamma) * div);
    }
    cv::LUT(src, cv::Mat(GradationBase, GradationMax, CV_8U, grad),  dst);
}
```
### 実行例
入力画像  
![入力画像](/実行例/閾値処理/0041_cropped.jpg)
出力画像  
![出力画像](/実行例/閾値処理/0041_thresholded.jpg)

## 見開き裁断
中心から縱に分割する。右綴じか左綴じかで格納順を変える。
``` cpp
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
```
## 二色化

THRESH_BINARYを使う。裏地の透けた頁が不気味になるので、大津の方法は使わずに固定値にした方がよい。

## 外枠領域の裁ち落とし

### 切り出し処理

先に作成した関数を使ってエッジを調整したのち、findContoursとboundingRectを利用して輪郭を検出する。
``` cpp
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
```

### 候補の絞り込み

小さな集合を除外する。
``` cpp
vector<Rect> CImageOperation::FindBoundRectangle(Mat& raw, vector<vector<cv::Point>> contours)
{
    int i;
    int minimum_size = 40;
    int minimum_ratio = 10;
    vector<Rect> lined_rect;
    vector<Rect> bound_rect;
    int width_cut = raw.size().width/ minimum_ratio;
    int height_cut = raw.size().height/ minimum_ratio;

    do {
        for (i = 0; i < contours.size(); ++i) {
            if (contours.at(i).size() > minimum_size) {
                lined_rect.push_back(boundingRect(contours.at(i)));
            }
        }
        for (i = 0; i < lined_rect.size(); ++i) {
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
```

### 最外矩形の作成
画面端を含む輪郭を除外しつつ、最内座標と最外座標から矩形を作成する。
``` cpp
Rect CImageOperation::CreateExtimusRectangle(vector<Rect> bound_rect)
{
    Rect extimus_rectangle{};
    extimus_rectangle.x = numeric_limits<int>::max();
    extimus_rectangle.y = numeric_limits<int>::max();

    int dextimus = 0;
    int infimus = 0;
    int i;
    for (i = 0; i < bound_rect.size(); ++i) {
        /*cv::Rect自体の幅・高さは用いずに最内座標と最外座標から矩形を作る。*/
        if ((bound_rect.at(i).x == 0) || bound_rect.at(i).y == 0)continue;
        int right = bound_rect.at(i).x + bound_rect.at(i).width;
        int bottom = bound_rect.at(i).y + bound_rect.at(i).height;
        if (right > dextimus) dextimus = right;
        if (bottom > infimus) infimus = bottom;
        if (bound_rect.at(i).x < extimus_rectangle.x)extimus_rectangle.x = bound_rect.at(i).x;
        if (bound_rect.at(i).y < extimus_rectangle.y)extimus_rectangle.y = bound_rect.at(i).y;
    }
    if (extimus_rectangle.x != numeric_limits<int>::max()) {
        extimus_rectangle.width = dextimus - extimus_rectangle.x;
        extimus_rectangle.height = infimus - extimus_rectangle.y;
    }

    /*端を含まぬ候補がない場合は面積で再探索*/
    if (extimus_rectangle.width == 0) {
        double largest_area = 0;
        int index = 0;
        for (i = 0; i < bound_rect.size(); ++i) {
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
```
### 天地左右切り落とし
どうしても前小口が含まれるので、これを切り落とす。
``` cpp
void CImageOperation::EliminateTrimmingArea(cv::Rect& rectangle)
{
    int cut_x = rectangle.width / 80;
    int cut_y = rectangle.height / 25;
    rectangle.x += cut_x;
    rectangle.y += cut_y;
    rectangle.width -= 2 * cut_x;
    rectangle.height -= 2 * cut_y;
}
```

### 幅の微調整
喉が概ね中心にあることを利用しての幅の微調整。
あくまで補助的な調整なので条件は厳しめに。
``` cpp
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

}
```

### 実行例

入力画像  
![入力画像](/実行例/切り落とし/0041.jpg)
検出画像  
![検出画像](/実行例/切り落とし/0041_rectangle.jpg)
切り出し画像  
![切り出し画像](/実行例/切り落とし/0041_cropped.jpg)

## 手動切り出し
大体はうまくいくが、それでも前小口を含んでしまうことがある。
ちょっとした調整にはselectROIを使う。
``` cpp
cv::Rect CImageOperation::SelectRegionOfInterst(cv::Mat& src)
{
    cv::Rect selected_rectangle;
    cv::namedWindow("select", cv::WINDOW_NORMAL | cv::WINDOW_KEEPRATIO);
    do {
        selected_rectangle = cv::selectROI("select", src);
    } while (selected_rectangle.empty());
    cv::destroyWindow("select");
    return selected_rectangle;
}
```
