/* 
 *  Author : Tomas Vojir
 *  Date   : 2013-06-05
 *  Desc   : Simple class for parsing VOT inputs and providing 
 *           interface for image loading and storing output.
 */ 

#ifndef CPP_VOT_H
#define CPP_VOT_H

#include <string>
#include <fstream>
#include <iostream>
#include <opencv2/opencv.hpp>


// Bounding box type
/* format:
    2 3
    1 4
*/
typedef struct {
    float x1;   //bottom left
    float y1;
    float x2;   //top left
    float y2;
    float x3;   //top right
    float y3;
    float x4;   //bottom right
    float y4;
} VOTPolygon;

class VOT
{
public:
    VOT(const std::string & region_file, const std::string & images, const std::string & output, const std::string & segmentations = "")
    {
        p_region_stream.open(region_file.c_str());
        p_polygon_ptr = 0;
        while (!p_region_stream.eof() && p_region_stream.is_open()) {
            VOTPolygon p;
            std::string line;
            std::getline(p_region_stream, line);
            if (line.size() == 0)
                continue;
            std::vector<float> numbers;
            std::istringstream s(line);
            float x;
            char ch;
            while (s >> x) {
                numbers.push_back(x);
                s >> ch;
            }
            if (numbers.size() == 4) {
                float x = numbers[0], y = numbers[1], w = numbers[2], h = numbers[3];
                p.x1 = x;
                p.y1 = y + h;
                p.x2 = x;
                p.y2 = y;
                p.x3 = x + w;
                p.y3 = y;
                p.x4 = x + w;
                p.y4 = y + h;
            } else if (numbers.size() == 8) {
                p.x1 = numbers[0];
                p.y1 = numbers[1];
                p.x2 = numbers[2];
                p.y2 = numbers[3];
                p.x3 = numbers[4];
                p.y3 = numbers[5];
                p.x4 = numbers[6];
                p.y4 = numbers[7];
            } else {
                std::cerr << "Error loading region from file - unknow format " << region_file << "!" << std::endl;
                p.x1 = 0;
                p.y1 = 0;
                p.x2 = 0;
                p.y2 = 0;
                p.x3 = 0;
                p.y3 = 0;
                p.x4 = 0;
                p.y4 = 0;
            }
            p_polygons.push_back(p);
        }

        std::cout << "VOT toolkit: loaded " << p_polygons.size() << " ground truth regions." << std::endl;

        p_images_stream.open(images.c_str());
        if (!p_images_stream.is_open())
            std::cerr << "Error loading image file " << images << "!" << std::endl;

        p_output_stream.open(output.c_str());
        if (!p_output_stream.is_open())
            std::cerr << "Error opening output file " << output << "!" << std::endl;

        if (!segmentations.empty()) {
            p_segmentation_stream.open(segmentations);
        }
    }

    ~VOT()
    {
        p_region_stream.close();
        p_images_stream.close();
        p_output_stream.close();
    }

    inline cv::Rect getNextRectangle() {
        cv::Rect rect(0,0,0,0);
        if (p_polygon_ptr < p_polygons.size()){
            // read init box from ground truth file
            VOTPolygon initPolygon = getNextPolygon();
            float x1 = std::min(initPolygon.x1, std::min(initPolygon.x2, std::min(initPolygon.x3, initPolygon.x4)));
            float x2 = std::max(initPolygon.x1, std::max(initPolygon.x2, std::max(initPolygon.x3, initPolygon.x4)));
            float y1 = std::min(initPolygon.y1, std::min(initPolygon.y2, std::min(initPolygon.y3, initPolygon.y4)));
            float y2 = std::max(initPolygon.y1, std::max(initPolygon.y2, std::max(initPolygon.y3, initPolygon.y4)));
            rect = cv::Rect(x1, y1, x2-x1, y2-y1);
        }
        return rect;
    }

    inline VOTPolygon getNextPolygon()
    {
        if (p_polygon_ptr < p_polygons.size())
            return p_polygons[p_polygon_ptr++];
        else {
            VOTPolygon p;
            p.x1 = 0;
            p.y1 = 0;
            p.x2 = 0;
            p.y2 = 0;
            p.x3 = 0;
            p.y3 = 0;
            p.x4 = 0;
            p.y4 = 0;
            return p;
        }
    }


    inline void outputBoundingBox(const cv::Rect & bbox)
    {
        p_output_stream << bbox.x << "," << bbox.y << ",";
        p_output_stream << bbox.width << "," << bbox.height << std::endl;
    }

    inline void outputPolygon(const VOTPolygon & poly)
    {
      p_output_stream << poly.x1 << "," << poly.y1 << ",";
      p_output_stream << poly.x2 << "," << poly.y2 << ",";
      p_output_stream << poly.x3 << "," << poly.y3 << ",";
      p_output_stream << poly.x4 << "," << poly.y4 << std::endl;
    }

    inline int getNextImage(cv::Mat & img)
    {
        if (p_images_stream.eof() || !p_images_stream.is_open())
                return -1;

        std::string line;
        std::getline (p_images_stream, line);
    	if (line.empty() && p_images_stream.eof()) return -1;
        img = cv::imread(line, CV_LOAD_IMAGE_COLOR);
     
        return 1;
    }

    inline int getNextSegmentation(cv::Mat & img)
    {
        if (p_segmentation_stream.eof() || !p_segmentation_stream.is_open())
            return -1;

        std::string line;
        std::getline (p_segmentation_stream, line);
        if (line.empty() && p_segmentation_stream.eof()) return -1;
        img = cv::imread(line, CV_LOAD_IMAGE_UNCHANGED);
        return 1;
    }

private:
    std::vector<VOTPolygon> p_polygons;
    size_t p_polygon_ptr;
    std::ifstream p_region_stream;
    std::ifstream p_images_stream;
    std::ifstream p_segmentation_stream;
    std::ofstream p_output_stream;

};

#endif //CPP_VOT_H
