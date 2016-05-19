//
// Created by vojirtom on 4/18/16.
// baseline from opencv/samples/cpp/grabcut.cpp
//

#ifndef GRABCUT_ANNOTATION_GRABCUT_APP_H
#define GRABCUT_ANNOTATION_GRABCUT_APP_H

#include <opencv2/opencv.hpp>

class Grabcut_app
{
public:
    //control vars
    bool m_overlay = false;
    bool m_show_enclosing_rest = false;
    bool m_validation = false;
    double m_valid_ratio_opacity = 1.0;
    double m_image_opacity = 0.3;
    int m_radius = 2;

    //minimum area rotated rectangle for the segmentation
    cv::RotatedRect m_enclosing_rect;

    void reset();
    void setImageAndWinName( const cv::Mat& _image, const std::string& _winName );
    void showImage(int number = -1);
    void mouseClick( int event, int x, int y, int flags, void* param );
    void nextIter();
    cv::Mat get_segmentation();
    void predict_background(cv::Mat & img0, cv::Mat & img1, cv::Mat & mask, cv::Rect & prev_bbox, bool uncertain = false);
    void set_rectangle(const cv::Rect & rect, bool uncertain = false);

    // Should be called after setImageAndWinName and setRectInMask
    void set_mask(const cv::Mat & mask);

private:
    //consts. and vars for plotting
    const cv::Scalar P_RED = cv::Scalar(0,0,255);
    const cv::Scalar P_PINK = cv::Scalar(230,130,255);
    const cv::Scalar P_BLUE = cv::Scalar(255,0,0);
    const cv::Scalar P_LIGHTBLUE = cv::Scalar(255,255,160);
    const cv::Scalar P_GREEN = cv::Scalar(0,255,0);
    static const int p_thickness = -1;
    int p_number = -1;

    //consts for keyboard inputs
    const int P_BGD_KEY = cv::EVENT_FLAG_CTRLKEY;
    const int P_FGD_KEY = cv::EVENT_FLAG_SHIFTKEY;

    //control consts
    enum{ NOT_SET = 0, IN_PROCESS = 1, SET = 2 };
    uchar p_rect_state, p_labeling_state, p_pr_labeling_state;
    bool p_is_initialized;

    //ptrs for displaying
    const std::string* p_win_name;
    const cv::Mat* p_image;

    //mats for grabcut
    cv::Mat p_mask, p_bgdModel, p_fgdModel;
    cv::Mat p_mask_valid_fg, p_mask_valid_bg;
    cv::Mat p_mask_cached;
    cv::Mat p_res_cached;
    int p_disp_counter = 0;

    double p_rect_size_ratio = 1.1;
    double p_roi_rect_size_ratio = 2.0;
    cv::Rect p_rect;
    cv::Rect p_roi_rect;

    std::vector<cv::Point> p_fgdPxls, p_bgdPxls, p_prFgdPxls, p_prBgdPxls;

    void setRectInMask();
    void setLblsInMask( int flags, cv::Point p, bool isPr );
    void getBinMask( const cv::Mat & comMask, cv::Mat & binMask);
    void show_temporal_pts(const cv::Point & pt, int flags);

    cv::Mat cyclshift(const cv::Mat &patch, int x_rot, int y_rot);
    void enlarge_bbox(cv::Rect & bbox, double expand_factor);
};


#endif //GRABCUT_ANNOTATION_GRABCUT_APP_H
