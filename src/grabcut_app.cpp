//
// Created by vojirtom on 4/18/16.
//

#include "grabcut_app.h"

void Grabcut_app::getBinMask( const cv::Mat & comMask, cv::Mat & binMask)
{
    if( comMask.empty() || comMask.type()!=CV_8UC1 )
        std::cout << "comMask is empty or has incorrect type (not CV_8UC1)\n";
    if( binMask.empty() || binMask.rows!=comMask.rows || binMask.cols!=comMask.cols )
        binMask.create( comMask.size(), CV_8UC1 );
    binMask = comMask & 1;

    cv::Mat mask_tmp = binMask * 255;
    binMask.setTo(0);

    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;
    cv::findContours(mask_tmp, contours, hierarchy, CV_RETR_TREE,
                     CV_CHAIN_APPROX_NONE, cv::Point(0, 0));

    //close segmentations (remove holes) + filter segmentations smaller than XX px
    std::vector<double> areas(contours.size(), 0);
    double max_area = 0;
    for (size_t i = 0, s = contours.size(); i < s; i++) {
        double a = cv::contourArea(contours[i], false);  //  Find the area of contour
        if (a > max_area)
            max_area = a;
        areas[i] = a;
    }

    double thr = 0.05*max_area;
    for (size_t i = 0, s = contours.size(); i < s; i++) {
        if (areas[i] > thr) {
            cv::drawContours(binMask, contours, i, cv::Scalar(1), CV_FILLED);
        }
    }
    cv::dilate(binMask, binMask, cv::Mat());
    cv::erode(binMask, binMask, cv::Mat());
}

void Grabcut_app::reset()
{
    if( !p_mask.empty() )
        p_mask.setTo(cv::Scalar::all(cv::GC_BGD));
    p_bgdPxls.clear(); p_fgdPxls.clear();
    p_prBgdPxls.clear();  p_prFgdPxls.clear();

    p_is_initialized = false;
    p_rect_state = NOT_SET;
    p_labeling_state = NOT_SET;
    p_pr_labeling_state = NOT_SET;
}

void Grabcut_app::setImageAndWinName( const cv::Mat & _image, const std::string& _winName  )
{
    if( _image.empty() || _winName.empty() )
        return;
    p_image = &_image;
    p_win_name = &_winName;
    p_mask.create( p_image->size(), CV_8UC1);
    p_mask_valid_fg.create( p_image->size(), CV_8UC1);
    p_mask_valid_bg.create( p_image->size(), CV_8UC1);
    reset();
}

void Grabcut_app::set_mask(const cv::Mat & mask)
{
    //NOTE 2016-05-16 16:36:44+02:00 : how to do it better ?

    //create mask of inside foreground using erode and background using dilate
    cv::Mat outside_contour(mask.size(), CV_8UC1), inner_contour(mask.size(), CV_8UC1);
    outside_contour.setTo(0);
    inner_contour.setTo(0);

    cv::Mat element = cv::getStructuringElement(cv::MORPH_RECT,
                                                cv::Size(9, 9));
    cv::dilate(mask, outside_contour, element);
    cv::threshold(outside_contour, outside_contour, 0, 255, cv::THRESH_BINARY_INV);

    cv::erode(mask, inner_contour, element);
    cv::threshold(inner_contour, inner_contour, 0, 255, cv::THRESH_BINARY);

    cv::Mat zeros = cv::Mat::zeros(p_mask.size(), CV_8UC1);
    cv::Mat ones = cv::Mat::ones(p_mask.size(), CV_8UC1);
    zeros.copyTo(p_mask, outside_contour);
    ones.copyTo(p_mask, inner_contour);
    p_is_initialized = true;
}

void Grabcut_app::showImage(int number)
{
    if( p_image->empty() || p_win_name->empty() )
        return;

    if (number >= 0)
        p_number = number;

    if( !p_is_initialized )
        p_image->copyTo( p_res_cached );
    else
    {
        if (m_overlay)
            p_res_cached = (*p_image).clone();
        else
            p_res_cached = m_image_opacity*(*p_image);

        p_image->copyTo( p_res_cached, p_mask_cached );
    }

    std::vector<cv::Point>::const_iterator it;
    cv::Mat circle_overlay = p_res_cached.clone();

    // show size of marking tool
    for( it = p_bgdPxls.begin(); it != p_bgdPxls.end(); ++it )
        cv::circle( circle_overlay, *it, m_radius, P_BLUE, p_thickness );
    for( it = p_fgdPxls.begin(); it != p_fgdPxls.end(); ++it )
        cv::circle( circle_overlay, *it, m_radius, P_RED, p_thickness );
    for( it = p_prBgdPxls.begin(); it != p_prBgdPxls.end(); ++it )
        cv::circle( circle_overlay, *it, m_radius, P_LIGHTBLUE, p_thickness );
    for( it = p_prFgdPxls.begin(); it != p_prFgdPxls.end(); ++it )
        cv::circle( circle_overlay, *it, m_radius, P_PINK, p_thickness );

    if( (p_rect_state == IN_PROCESS || p_rect_state == SET) && !m_validation ) {
        cv::rectangle(circle_overlay, cv::Point(p_rect.x, p_rect.y), cv::Point(p_rect.x + p_rect.width, p_rect.y + p_rect.height), P_GREEN, 1);
        //cv::rectangle(circle_overlay, cv::Point(p_roi_rect.x, p_roi_rect.y), cv::Point(p_roi_rect.x + p_roi_rect.width, p_roi_rect.y + p_roi_rect.height), P_RED, 1);
    }

    p_res_cached = 0.5*p_res_cached + 0.5*circle_overlay;


    if (m_show_enclosing_rest){
        cv::Point2f vertices[4];
        m_enclosing_rect.points(vertices);
        for (int i = 0; i < 3; i++)
            line(p_res_cached, vertices[i], vertices[(i+1)], P_BLUE);
        line(p_res_cached, vertices[3], vertices[0], P_BLUE);
    }

    if (m_validation) {
        cv::Mat b(p_res_cached.size(), CV_8UC1), g(p_res_cached.size(), CV_8UC1), r(p_res_cached.size(), CV_8UC1);
        std::vector<cv::Mat> mat_arr = {b, g, r};
        cv::split(p_res_cached, mat_arr);
        double ratio = m_valid_ratio_opacity;
        if (!m_overlay)
            ratio = 0.5*ratio;

        cv::max(g, p_mask_valid_fg*ratio, g);
        cv::max(r, p_mask_valid_bg*ratio, r);
        cv::merge(mat_arr, p_res_cached);
    }

    if (p_number >= 0) {
        std::stringstream s; std::string num;
        s << "#" << p_number;
        s >> num;
        cv::putText(p_res_cached, num, cvPoint(5,15), cv::FONT_HERSHEY_COMPLEX_SMALL, 0.8, cvScalar(218,218,40), 1, CV_AA);
    }

    //show size of the mark tool
    std::stringstream s; std::string num;
    s << "r:" << m_radius;
    s >> num;
    cv::putText(p_res_cached, num, cvPoint(p_res_cached.cols-55,15), cv::FONT_HERSHEY_COMPLEX_SMALL, 0.8, cvScalar(218,218,40), 1, CV_AA);

    cv::imshow( *p_win_name, p_res_cached );
    p_disp_counter = 0;
}

void Grabcut_app::setRectInMask()
{
    CV_Assert( !p_mask.empty() );
    p_mask.setTo( cv::GC_BGD );

    p_rect.x = std::max(0, p_rect.x);
    p_rect.y = std::max(0, p_rect.y);
    if (p_rect.x + p_rect.width >= p_image->cols)
        p_rect.width = p_rect.width - (p_rect.x + p_rect.width - p_image->cols) - 1;
    if (p_rect.y + p_rect.height >= p_image->rows)
        p_rect.height = p_rect.height - (p_rect.y + p_rect.height - p_image->rows) -1;

    p_roi_rect.x = std::max(0, p_roi_rect.x);
    p_roi_rect.y = std::max(0, p_roi_rect.y);
    if (p_roi_rect.x + p_roi_rect.width >= p_image->cols)
        p_roi_rect.width = p_roi_rect.width - (p_roi_rect.x + p_roi_rect.width - p_image->cols) - 1;
    if (p_roi_rect.y + p_roi_rect.height >= p_image->rows)
        p_roi_rect.height = p_roi_rect.height - (p_roi_rect.y + p_roi_rect.height - p_image->rows) -1;

    (p_mask(p_rect)).setTo( cv::Scalar(cv::GC_PR_FGD) );

    getBinMask(p_mask, p_mask_cached);
}

void Grabcut_app::setLblsInMask( int flags, cv::Point p, bool isPr )
{
    std::vector<cv::Point> *bpxls, *fpxls;
    uchar bvalue, fvalue;
    if( !isPr ) {
        bpxls = &p_bgdPxls;
        fpxls = &p_fgdPxls;
        bvalue = cv::GC_BGD;
        fvalue = cv::GC_FGD;
    } else {
        bpxls = &p_prBgdPxls;
        fpxls = &p_prFgdPxls;
        bvalue = cv::GC_PR_BGD;
        fvalue = cv::GC_PR_FGD;
    }

    if( flags & P_BGD_KEY ) {
        bpxls->push_back(p);
        cv::circle( p_mask, p, m_radius, bvalue, p_thickness );
    }

    if( flags & P_FGD_KEY ) {
        fpxls->push_back(p);
        cv::circle( p_mask, p, m_radius, fvalue, p_thickness );
    }
}

void Grabcut_app::mouseClick( int event, int x, int y, int flags, void* )
{
    bool isb = (flags & P_BGD_KEY) != 0,
         isf = (flags & P_FGD_KEY) != 0;
    switch( event )
    {
        case cv::EVENT_LBUTTONDOWN: // set rect or GC_BGD(GC_FGD) labels
        {
            if( p_rect_state == NOT_SET && !isb && !isf ) {
                p_rect_state = IN_PROCESS;
                p_rect = cv::Rect( x, y, 1, 1 );
            }

            if ( (isb || isf) && p_rect_state == SET )
                p_labeling_state = IN_PROCESS;
        }
            break;
        case cv::EVENT_RBUTTONDOWN: // set GC_PR_BGD(GC_PR_FGD) labels
        {
            if ( (isb || isf) && p_rect_state == SET )
                p_pr_labeling_state = IN_PROCESS;
        }
            break;
        case cv::EVENT_LBUTTONUP:
            if( p_rect_state == IN_PROCESS ) {
                p_rect = cv::Rect( cv::Point(p_rect.x, p_rect.y), cv::Point(x,y) );
                p_roi_rect = p_rect;
                enlarge_bbox(p_roi_rect, p_roi_rect_size_ratio);
                p_rect_state = SET;
                setRectInMask();
                CV_Assert( p_bgdPxls.empty() && p_fgdPxls.empty() && p_prBgdPxls.empty() && p_prFgdPxls.empty() );
                showImage();
            }

            if( p_labeling_state == IN_PROCESS ) {
                setLblsInMask(flags, cv::Point(x,y), false);
                p_labeling_state = SET;
                showImage();
            }
            break;
        case cv::EVENT_RBUTTONUP:
            if( p_pr_labeling_state == IN_PROCESS ) {
                setLblsInMask(flags, cv::Point(x,y), true);
                p_pr_labeling_state = SET;
                showImage();
            }
            break;
        case cv::EVENT_MOUSEMOVE:
            if( p_rect_state == IN_PROCESS ) {
                p_rect = cv::Rect( cv::Point(p_rect.x, p_rect.y), cv::Point(x,y) );
                CV_Assert( p_bgdPxls.empty() && p_fgdPxls.empty() && p_prBgdPxls.empty() && p_prFgdPxls.empty() );
                showImage();
            } else if( p_labeling_state == IN_PROCESS ) {
                setLblsInMask(flags, cv::Point(x,y), false);
                if (p_disp_counter % 2 == 0) show_temporal_pts(cv::Point(x,y), flags);
            } else if( p_pr_labeling_state == IN_PROCESS ) {
                setLblsInMask(flags, cv::Point(x,y), true);
                if (p_disp_counter % 2 == 0) show_temporal_pts(cv::Point(x,y), flags);
            }
            p_disp_counter++;
            break;
    }
}

void Grabcut_app::show_temporal_pts(const cv::Point & pt, int flags)
{
    int xmin = std::max(pt.x - m_radius, 0);
    int xmax = std::min(pt.x + m_radius, p_res_cached.cols);
    int ymin = std::max(pt.y - m_radius, 0);
    int ymax = std::min(pt.y + m_radius, p_res_cached.rows);

    cv::Scalar color = (flags & P_BGD_KEY) ? P_BLUE : P_RED;

    for (int y = ymin; y < ymax; ++y){
        cv::Vec3b * pixel = p_res_cached.ptr<cv::Vec3b>(y);
        for (int x = xmin; x < xmax; ++x){
            pixel[x][2] = static_cast<uchar>(0.8*pixel[x][2] + 0.2*color[2]);
            pixel[x][1] = static_cast<uchar>(0.8*pixel[x][1] + 0.2*color[1]);;
            pixel[x][0] = static_cast<uchar>(0.8*pixel[x][0] + 0.2*color[0]);;
        }
    }

    cv::imshow( *p_win_name, p_res_cached );
}

void Grabcut_app::nextIter()
{
    if( p_is_initialized )
        cv::grabCut((*p_image)(p_roi_rect), p_mask(p_roi_rect), cv::Rect(p_rect.x - p_roi_rect.x, p_rect.y - p_roi_rect.y, p_rect.width, p_rect.height),
                    p_bgdModel, p_fgdModel, 1, cv::GC_EVAL);
    else
    {
        if( p_rect_state != SET )
            return;

        if( p_labeling_state == SET || p_pr_labeling_state == SET)
            cv::grabCut( (*p_image)(p_roi_rect), p_mask(p_roi_rect), p_rect, p_bgdModel, p_fgdModel, 1, cv::GC_INIT_WITH_MASK );
        else {
            cv::grabCut((*p_image)(p_roi_rect), p_mask(p_roi_rect), cv::Rect(p_rect.x-p_roi_rect.x, p_rect.y-p_roi_rect.y, p_rect.width, p_rect.height),
                        p_bgdModel, p_fgdModel, 1, cv::GC_INIT_WITH_RECT);
        }

        p_is_initialized = true;
    }

    p_bgdPxls.clear(); p_fgdPxls.clear();
    p_prBgdPxls.clear(); p_prFgdPxls.clear();

    //clear contour
    getBinMask(p_mask, p_mask_cached);

    //create mask of inside foreground using erode and background using dilate
    p_mask_valid_fg.setTo(0);
    p_mask_valid_bg.setTo(0);
    cv::Mat element = cv::getStructuringElement(cv::MORPH_RECT,
                                                cv::Size(9, 9));
    cv::dilate(p_mask_cached, p_mask_valid_bg, element);
    cv::threshold(p_mask_valid_bg(p_roi_rect), p_mask_valid_bg(p_roi_rect),
                  0, 255, cv::THRESH_BINARY_INV);

    cv::erode(p_mask_cached, p_mask_valid_fg, element);
    cv::threshold(p_mask_valid_fg, p_mask_valid_fg, 0, 255,
                  cv::THRESH_BINARY);

}

cv::Mat Grabcut_app::get_segmentation()
{
    if( p_image->empty() || p_win_name->empty() )
        return cv::Mat();

    cv::Mat binMask;
    if( p_is_initialized )
        getBinMask( p_mask, binMask );
    binMask = binMask * 255;

    return binMask;
}

void Grabcut_app::set_rectangle(const cv::Rect & _rect, bool uncertain)
{
    p_rect = _rect;
    p_roi_rect = _rect;

    p_rect_state = SET;

    if (uncertain)
        enlarge_bbox(p_rect, p_rect_size_ratio+0.2);
    else
        enlarge_bbox(p_rect, p_rect_size_ratio);

    enlarge_bbox(p_roi_rect, p_roi_rect_size_ratio);

    setRectInMask();
}

void Grabcut_app::predict_background(cv::Mat & img0, cv::Mat & img1, cv::Mat & mask0, cv::Rect & _prev_bbox, bool uncertain)
{
    p_labeling_state = SET;
    cv::Mat i0_gray, i1_gray;
    cv::cvtColor(img0, i0_gray, CV_BGR2GRAY);
    cv::cvtColor(img1, i1_gray, CV_BGR2GRAY);
    std::vector<cv::Point2f> corners_i0, corners_i1;

    cv::Mat rect_mask = p_mask.clone();
    rect_mask.setTo(0);
    cv::Rect prev_bbox = _prev_bbox;
    if (uncertain)
        enlarge_bbox(prev_bbox, p_rect_size_ratio+0.2);
    else
        enlarge_bbox(prev_bbox, p_rect_size_ratio);
    prev_bbox.x = std::max(0, prev_bbox.x);
    prev_bbox.y = std::max(0, prev_bbox.y);
    if (prev_bbox.x + prev_bbox.width >= p_image->cols)
        prev_bbox.width = prev_bbox.width - (prev_bbox.x + prev_bbox.width - p_image->cols) - 1;
    if (prev_bbox.y + prev_bbox.height >= p_image->rows)
        prev_bbox.height = prev_bbox.height - (prev_bbox.y + prev_bbox.height - p_image->rows) -1;
    (rect_mask(prev_bbox)).setTo( 255 );

    cv::goodFeaturesToTrack(i0_gray, corners_i0, 250, 0.01, 5, rect_mask);
    std::vector<uchar> status;
    std::vector<float> err;
    cv::calcOpticalFlowPyrLK(i0_gray, i1_gray, corners_i0, corners_i1, status, err, cv::Size(11, 11), 4);

    //propagate whole segmentation as soft foreground seeds
    cv::Point shift = cv::Point(p_rect.x+p_rect.width/2, p_rect.y+p_rect.height/2) - cv::Point(prev_bbox.x+prev_bbox.width/2, prev_bbox.y+prev_bbox.height/2);
    p_mask.setTo(cv::GC_BGD);
    p_mask(p_rect).setTo(cv::GC_PR_BGD);
    cv::Mat shifted_mask = cyclshift(mask0, shift.x, shift.y);
    shifted_mask = shifted_mask & 1;

    //add uncertainty to mask
    int diletation_size = 3;
    if (uncertain)
        diletation_size = 9;
    cv::Mat element = cv::getStructuringElement( cv::MORPH_RECT, cv::Size(diletation_size, diletation_size));
    cv::dilate( shifted_mask, shifted_mask, element );

    p_mask.setTo(cv::GC_PR_FGD, shifted_mask);

    //cv::imshow("mask-aprior", mask*100);

    for (size_t i = 0; i < status.size(); ++i) {
        if (status[i] == 1 && p_rect.contains(corners_i1[i])) {
            if (mask0.at<uchar>(corners_i0[i].y, corners_i0[i].x) == 0) {
                //prob bg pixel
                cv::circle( p_mask, corners_i1[i], 1, cv::GC_BGD, p_thickness );
            } else {
                cv::circle( p_mask, corners_i1[i], 1, cv::GC_FGD, p_thickness );
            }
        }
    }

    //test if there is enough predicted pixels
    double area = cv::sum(p_mask & 1).val[0];
    if (area/p_rect.area() < 0.1)
        p_labeling_state = NOT_SET;

    //cv::imshow("mast-klt_predict", mask*100);

}


cv::Mat Grabcut_app::cyclshift(const cv::Mat &patch, int x_rot, int y_rot)
{
    cv::Mat rot_patch(patch.size(), CV_8UC1);
    rot_patch.setTo(0);
    cv::Mat tmp_x_rot(patch.size(), CV_8UC1);
    tmp_x_rot.setTo(0);

    //circular rotate x-axis
    if (x_rot < 0) {
        //move part that does not rotate over the edge
        cv::Range orig_range(-x_rot, patch.cols);
        cv::Range rot_range(0, patch.cols - (-x_rot));
        patch(cv::Range::all(), orig_range).copyTo(tmp_x_rot(cv::Range::all(), rot_range));

        //rotated part
        orig_range = cv::Range(0, -x_rot);
        rot_range = cv::Range(patch.cols - (-x_rot), patch.cols);
        patch(cv::Range::all(), orig_range).copyTo(tmp_x_rot(cv::Range::all(), rot_range));
    }else if (x_rot > 0){
        //move part that does not rotate over the edge
        cv::Range orig_range(0, patch.cols - x_rot);
        cv::Range rot_range(x_rot, patch.cols);
        patch(cv::Range::all(), orig_range).copyTo(tmp_x_rot(cv::Range::all(), rot_range));

        //rotated part
        orig_range = cv::Range(patch.cols - x_rot, patch.cols);
        rot_range = cv::Range(0, x_rot);
        patch(cv::Range::all(), orig_range).copyTo(tmp_x_rot(cv::Range::all(), rot_range));
    }else { //zero rotation
        //move part that does not rotate over the edge
        cv::Range orig_range(0, patch.cols);
        cv::Range rot_range(0, patch.cols);
        patch(cv::Range::all(), orig_range).copyTo(tmp_x_rot(cv::Range::all(), rot_range));
    }

    //circular rotate y-axis
    if (y_rot < 0) {
        //move part that does not rotate over the edge
        cv::Range orig_range(-y_rot, patch.rows);
        cv::Range rot_range(0, patch.rows - (-y_rot));
        tmp_x_rot(orig_range, cv::Range::all()).copyTo(rot_patch(rot_range, cv::Range::all()));

        //rotated part
        orig_range = cv::Range(0, -y_rot);
        rot_range = cv::Range(patch.rows - (-y_rot), patch.rows);
        tmp_x_rot(orig_range, cv::Range::all()).copyTo(rot_patch(rot_range, cv::Range::all()));
    }else if (y_rot > 0){
        //move part that does not rotate over the edge
        cv::Range orig_range(0, patch.rows - y_rot);
        cv::Range rot_range(y_rot, patch.rows);
        tmp_x_rot(orig_range, cv::Range::all()).copyTo(rot_patch(rot_range, cv::Range::all()));

        //rotated part
        orig_range = cv::Range(patch.rows - y_rot, patch.rows);
        rot_range = cv::Range(0, y_rot);
        tmp_x_rot(orig_range, cv::Range::all()).copyTo(rot_patch(rot_range, cv::Range::all()));
    }else { //zero rotation
        //move part that does not rotate over the edge
        cv::Range orig_range(0, patch.rows);
        cv::Range rot_range(0, patch.rows);
        tmp_x_rot(orig_range, cv::Range::all()).copyTo(rot_patch(rot_range, cv::Range::all()));
    }

    return rot_patch;
}

void Grabcut_app::enlarge_bbox(cv::Rect & bbox, double expand_factor)
{
    cv::Point center(bbox.x + bbox.width / 2,
                     bbox.y + bbox.height / 2);
    bbox.width = static_cast<int>(bbox.width * expand_factor);
    bbox.height = static_cast<int>(bbox.height * expand_factor);
    bbox.x = center.x - bbox.width / 2;
    bbox.y = center.y - bbox.height / 2;
}
