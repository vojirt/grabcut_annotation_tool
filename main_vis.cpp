//
// Created by vojirtom on 5/2/16.
//

#include <iostream>
#include <fstream>

#include <opencv2/opencv.hpp>

int main(int argc, char ** argv)
{
    std::cout << "\nThis program visualize segmentation\n"
            "Call:\n"
            "./annotation_vis <images.txt> <segmentation_images.txt>\n"
            "\nHot keys: \n"
                "\tany key (except ESC) - next image\n"
                "\tESC - quit the program\n" << std::endl;

    if (argc < 3) {
        std::cerr << "Input must be two text files!" << std::endl;
        return 1;
    }

    std::ifstream images_stream;
    std::ifstream segmentation_stream;

    images_stream.open(argv[1]);
    if (!images_stream.is_open())
        std::cerr << "Error loading image file " << argv[1] << "!" << std::endl;

    segmentation_stream.open(argv[2]);
    if (!segmentation_stream.is_open())
        std::cerr << "Error loading image file " << argv[2] << "!" << std::endl;

    cv::namedWindow("Visualization", cv::WINDOW_NORMAL);

    int frame_number = 0;

    while (true) {

        std::string line_image;
        std::getline (images_stream, line_image);
        if (line_image.empty() && images_stream.eof())
            break;

        cv::Mat img = cv::imread(line_image, CV_LOAD_IMAGE_COLOR);

        std::string line_segmentation;
        std::getline (segmentation_stream, line_segmentation);
        if (line_segmentation.empty() && segmentation_stream.eof())
            break;

        cv::Mat segmentation = cv::imread(line_segmentation, CV_LOAD_IMAGE_GRAYSCALE);

        //find rectangle enclosing segmentation for roi
        cv::Rect roi;
        std::vector<std::vector<cv::Point>> contours;
        std::vector<cv::Vec4i> hierarchy;
        cv::findContours( segmentation, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_NONE, cv::Point(0, 0));
        if (contours.size() > 0){
            cv::RotatedRect enclosing_rect = cv::minAreaRect(contours[0]);
            roi = enclosing_rect.boundingRect();

            cv::Point center(roi.x + roi.width / 2,
                             roi.y + roi.height / 2);
            roi.width = static_cast<int>(roi.width * 2);
            roi.height = static_cast<int>(roi.height * 2);
            roi.x = center.x - roi.width / 2;
            roi.y = center.y - roi.height / 2;

            roi.x = std::max(0, roi.x);
            roi.y = std::max(0, roi.y);
            roi.width = std::min(roi.width, img.cols-roi.x);
            roi.height = std::min(roi.height, img.rows-roi.y);

        } else {
            roi = cv::Rect(0, 0, img.cols-1, img.rows-1);
        }

        //create visualization
        cv::Mat res(roi.height, roi.width*2, CV_8UC3);
        res.setTo(0);
        //copy segmented image to left half
        img(roi).copyTo(res(cv::Rect(0,0, roi.width, roi.height)), segmentation(roi));

        //create contour visualization
        cv::Mat mask_valid_fg(segmentation.size(), CV_8UC1), mask_valid_bg(segmentation.size(), CV_8UC1);
        mask_valid_fg.setTo(0);
        mask_valid_bg.setTo(0);
        cv::Mat element = cv::getStructuringElement( cv::MORPH_RECT, cv::Size(9, 9));
        cv::dilate( segmentation, mask_valid_bg, element );
        cv::threshold( mask_valid_bg(roi), mask_valid_bg(roi), 0, 255, cv::THRESH_BINARY_INV);

        cv::erode(segmentation, mask_valid_fg, element);
        cv::threshold( mask_valid_fg, mask_valid_fg, 0, 255, cv::THRESH_BINARY);

        cv::Mat b(img.size(), CV_8UC1), g(img.size(), CV_8UC1), r(img.size(), CV_8UC1);
        std::vector<cv::Mat> mat_arr = {b, g, r};
        cv::split(img*0.7, mat_arr);
        cv::max(g, mask_valid_fg*0.7, g);
        cv::max(r, mask_valid_bg*0.9, r);
        cv::merge(mat_arr, img);

        //copy segmented image to left half
        cv::Rect shifted_roi(roi.width, 0, roi.width, roi.height);
        img(roi).copyTo(res(shifted_roi));

        //show frame number
        std::stringstream s; std::string num;
        s << "#" << frame_number;
        s >> num;
        cv::putText(res, num, cvPoint(5,15), cv::FONT_HERSHEY_COMPLEX_SMALL, 0.6, cvScalar(218,218,40), 1, CV_AA);

        cv::imshow("Visualization", res);
        int c = cv::waitKey();
        if ((char) c == '\x1b') {
            break;
        }
    }


    return 0;
}