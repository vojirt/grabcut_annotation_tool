## Semi-automatic annotation tool

Code author : Tomas Vojir

________________

This is an implementation of a semi-automatic segmentation Grabcut algorithm for annotating a object segmentation
in a video sequence or in a single image. The alg. allows to seed initial rectangles to speed up the annotation.

For annotation of video sequence, there is a prediction mechanism which propagates the segmentation from 
previous frame to the consecutive frames to limit the human intervation to small correction.

It is free for research use. If you find it useful or use it in your research, please acknowledge my git repository.

The code depends on OpenCV 2.4+ library and is build via cmake toolchain.

_________________
Quick start guide

for linux: open terminal in the directory with the code

$ mkdir build; cd build; cmake .. ; make

This code compiles into binary **grabcut_annotation** for interactive segmentation 
and **annotation_vis** for visualization of the segmentation results

./grabcut_annotation *image_name*
- Select a rectangular area around the object you want to segment

./grabcut_annotation *images.txt* *groundtruth.txt*(file may be empty) *start_frame*=0
- Use VOT (http://www.votchallenge.net/) format  to annotate sequence from gt or by hand (expect images.txt and optionally groundtruth.txt)
    - images.txt list of sequence images with absolute path
    - groundtruth.txt bounding boxes for the files in images.txt in the format "top_left_x, top_left_y, width, height" or 
           four corner points listed clockwise starting from bottom left corner.

Hot keys:
- **ESC**: quit the program
- **r**: restore the original image
- **a**:  apply next iteration of grabcut
- **s**: save segmentation (go to the next img if vot format)
- **t**: toggle on/off full image opacity overlay in visualization
- **e**: toggle on/off enclosing rectangle in visualization
- **v**: toggle on/off validation visualization
- **-**, **+**: decrease/increase opacity of validation visualization
- **{**, **}**: decrease/increase opacity of image overlay
- **[**, **]**: decrease/increase size of the mark tool
- **f**: skip current frame
- **left mouse button**: set rectangle
- **CTRL+left mouse button**:  set BACKGROUND pixels
- **SHIFT+left mouse button**: set FOREGREOUND pixels

OUTPUT: 
- the output is saved as binary images to the *out/* directory (**must** be created manually before running the app) 
- *output.txt* file with four-corner format min area rotated rectangle fitted to the segmentation



./annotation_vis *images.txt* *segmentation_images.txt* *output_file=output_frame_numbers.txt*
- images.txt list of sequence images with absolute path
- segmentation_images.txt list of segmentation images (produced by *grabcut_annotation*) with absolute path
- output_file set default to output_frame_numbers.txt for storing selected frame numbers

Hot keys: 
- **ESC** - quit the program
- **n** - save currect frame number
- **any other key** - next image


_____________________________________
Copyright (c) 2016, Tomáš Vojíř

Permission to use, copy, modify, and distribute this software is hereby granted, 
provided that the above copyright notice and this permission notice 
appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

__________________
OpenCV license

[LICENCE: 3-clause BSD License](LICENSE_OpenCV.txt)

