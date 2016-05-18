//
// Created by vojirtom on 4/18/16.
//

#include "grabcut_app.h"
#include "vot.hpp"

#include <memory>


//TODO
//  GCAPP:
//      - refactor names
//      - flags for on/off features (control flags)


Grabcut_app gcapp;

void on_mouse(int event, int x, int y, int flags, void* param )
{
    gcapp.mouseClick( event, x, y, flags, param );
}

VOTPolygon rot_rect2vot_poly(cv::RotatedRect & rot_rect)
{
    VOTPolygon p;
    cv::Point2f vertices[4];
    rot_rect.points(vertices);
    p.x1 = vertices[0].x;
    p.y1 = vertices[0].y;
    p.x2 = vertices[1].x;
    p.y2 = vertices[1].y;
    p.x3 = vertices[2].x;
    p.y3 = vertices[2].y;
    p.x4 = vertices[3].x;
    p.y4 = vertices[3].y;
    return p;
}

int main( int argc, char** argv )
{

    std::cout << "\nThis program demonstrates GrabCut segmentation -- select an object in a region\n"
            "and then grabcut will attempt to segment it out.\n"
            "Call:\n"
            "./grabcut_annotation <image_name>\n"
            "\tSelect a rectangular area around the object you want to segment\n" <<
            "./grabcut_annotation <images.txt> <groundtruth.txt (file may be empty)> <start_frame=0>\n"
            "\tUse VOT format to annotate sequence from gt or by hand (expect images.txt and optionally groundtruth.txt)\n" <<
            "\nHot keys: \n"
            "\tESC - quit the program\n"
            "\tr - restore the original image\n"
            "\ta - apply next iteration of grabcut\n"
            "\ts - save segmentation (go to the next img if vot format)\n"
            "\tt - toggle on/off full image opacity overlay in visualization\n"
            "\te - toggle on/off enclosing rectangle in visualization\n"
            "\tv - toggle on/off validation visualization\n"
            "\t-, + - decrease/increase opacity of validation visualization\n"
            "\t{, } - decrease/increase opacity of image overlay\n"
            "\t[, ] - decrease/increase size of the mark tool\n"
            "\tf - skip current frame\n"
            "\n"
            "\tleft mouse button - set rectangle\n"
            "\n"
            "\tCTRL+left mouse button - set GC_BGD pixels\n"
            "\tSHIFT+left mouse button - set GC_FGD pixels\n"
            "\n" << std::endl;
//            "\tCTRL+right mouse button - set GC_PR_BGD pixels\n"
//            "\tSHIFT+right mouse button - set GC_PR_FGD pixels\n" << std::endl;

    const cv::string winName = "image";
    cv::Mat image, segmentation_mask;
    std::unique_ptr<VOT> vot;
    int output_file_number = 0;
    int segmentation_status = 0;

    if( argc == 2 ) {
        std::string filename = {argv[1]};
        image = cv::imread( filename, 1 );
    } else if (argc >= 3) {
        //using vot
        std::string seg_file = "";
        if (argc > 4)
            seg_file = argv[4];

        vot.reset(new VOT{argv[2], argv[1], "output.txt", seg_file});
        vot->getNextImage(image);
        segmentation_status = vot->getNextSegmentation(segmentation_mask);
    }

    int start_frame = 0;
    if (argc > 3)
        start_frame = atoi(argv[3]);

    for (int i = 0; i < start_frame; ++i) {
        output_file_number++;
        int status = vot->getNextImage(image);
        segmentation_status = vot->getNextSegmentation(segmentation_mask);
        if (status < 0) {
            std::cout << "Starting frame is larger than number of frames in the sequence!" << std::endl;
            return 1;
        }

        vot->getNextRectangle();
    }

    if( image.empty() ) {
        std::cout << "Error in loading image(s)!" << std::endl;
        return 1;
    }

    cv::namedWindow( winName, cv::WINDOW_NORMAL );
    cv::setMouseCallback( winName, on_mouse, 0);
    gcapp.setImageAndWinName( image, winName );

    cv::Rect bbox;
    if (vot) {
        bbox = vot->getNextRectangle();
        if (bbox.width > 0 && bbox.height > 0) {
            gcapp.set_rectangle(bbox);
            if (segmentation_status > 0)
                gcapp.set_mask(segmentation_mask);
            gcapp.nextIter();
        }
    }
    gcapp.showImage(output_file_number);

    bool quit = false;
    while (!quit) {
        int c = cv::waitKey(0);
        switch( (char) c ) {
            case '\x1b':{
                std::cout << "Exiting ..." << std::endl;
                quit = true;
                break;}
            case 'r':{
                gcapp.reset();
                gcapp.showImage();
                break;}
            case 'a':{
                gcapp.nextIter();
                gcapp.showImage();
                break;}
            case '-':{
                gcapp.m_valid_ratio_opacity -= 0.1;
                if (gcapp.m_valid_ratio_opacity < 0.3)
                    gcapp.m_valid_ratio_opacity = 0.3;
                gcapp.showImage();
                break;}
            case '+':{
                gcapp.m_valid_ratio_opacity += 0.1;
                if (gcapp.m_valid_ratio_opacity > 1.)
                    gcapp.m_valid_ratio_opacity = 1.;
                gcapp.showImage();
                break;}
            case '{':{
                gcapp.m_image_opacity -= 0.1;
                if (gcapp.m_image_opacity < 0.3)
                    gcapp.m_image_opacity = 0.3;
                gcapp.showImage();
                break;}
            case '}':{
                gcapp.m_image_opacity += 0.1;
                if (gcapp.m_image_opacity > 0.9)
                    gcapp.m_image_opacity = 0.9;
                gcapp.showImage();
                break;}
            case '[':{
                gcapp.m_radius -= 1;
                if (gcapp.m_radius < 2)
                    gcapp.m_radius = 2;
                gcapp.showImage();
                break;}
            case ']':{
                gcapp.m_radius += 1;
                if (gcapp.m_radius > 100)
                    gcapp.m_radius = 100;
                gcapp.showImage();
                break;}
            case 'e':{
                gcapp.m_show_enclosing_rest = !gcapp.m_show_enclosing_rest;
                gcapp.showImage();
                break;}
            case 't':{
                gcapp.m_overlay = !gcapp.m_overlay;
                gcapp.showImage();
                break;}
            case 'v':{
                gcapp.m_validation = !gcapp.m_validation;
                gcapp.showImage();
                break;}
            case 'f':{
                if (vot){
                    vot->outputPolygon(VOTPolygon());

                    int status = vot->getNextImage(image);
                    segmentation_status = vot->getNextSegmentation(segmentation_mask);
                    if (status > 0) {
                        output_file_number++;
                        gcapp.setImageAndWinName(image, winName);
                        bbox = vot->getNextRectangle();
                        if (bbox.width > 0 && bbox.height > 0) {
                            gcapp.set_rectangle(bbox);
                            if (segmentation_status > 0)
                                gcapp.set_mask(segmentation_mask);
                            gcapp.nextIter();
                        }
                        gcapp.showImage(output_file_number);
                    } else {
                        std::cout << "End of image sequence. Exiting ..." <<
                        std::endl;
                        quit = true;
                    }
                }else{
                    std::cout << "Exiting ..." << std::endl;
                    quit = true;
                }
                break;}
            case 's': {
                std::stringstream s;
                std::string save_filename;
                s << "out/"; s.fill('0'); s.width(8); s << std::right << output_file_number++; s << ".png"; s >> save_filename;
                cv::Mat segmentation = gcapp.get_segmentation();
                cv::imwrite(save_filename, segmentation);

                if (vot) {
                    //save bbox
                    vot->outputPolygon(rot_rect2vot_poly(gcapp.m_enclosing_rect));

                    cv::Rect prev_bbox = bbox;
                    cv::Mat prev_img = image.clone();
                    int status = vot->getNextImage(image);
                    segmentation_status = vot->getNextSegmentation(segmentation_mask);
                    if (status > 0) {
                        gcapp.setImageAndWinName(image, winName);
                        bbox = vot->getNextRectangle();
                        bool uncertain_bbox = false;
                        if (bbox.width > 0 && bbox.height > 0) {
                            gcapp.set_rectangle(bbox);
                        } else {
                            //no gt available for the sequence, using zero motion prediction
                            //NOTE: There can be used a more complicated dynamic model or a tracking
                            //      alg., but it is probably overkill, so instead the seeded rectangle
                            //      is enlarged a bit more
                            uncertain_bbox = true;
                            prev_bbox = gcapp.m_enclosing_rect.boundingRect();
                            gcapp.set_rectangle(prev_bbox, true);
                        }
                        //seed the initial mask with probable background seeds
                        if (segmentation_status > 0)
                            gcapp.set_mask(segmentation_mask);
                        else
                            gcapp.predict_background(prev_img, image, segmentation, prev_bbox, uncertain_bbox);
                        gcapp.nextIter();
                        gcapp.showImage(output_file_number);
                    } else {
                        std::cout << "End of image sequence. Exiting ..." <<
                        std::endl;
                        quit = true;
                    }
                } else {
                    cv::Point2f vertices[4];
                    gcapp.m_enclosing_rect.points(vertices);
                    std::cout << "Enclosing rectangle : ";
                    for (int i = 0; i < 4; i++)
                        std::cout << vertices[i] << ", ";
                    std::cout << std::endl;
                }
                break;}
            default:;
        }
    }

    cv::destroyWindow( winName );
    return 0;
}