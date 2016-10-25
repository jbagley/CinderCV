/*
 * Copyright (C) 2016 Art & Logic, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy 
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/Capture.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"

#include "CinderOpenCV.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class CincerCvApp : public App
{
public:
   void setup() override;
   void mouseDown( MouseEvent event ) override;
   void update() override;
   void draw() override;
   
private:
   CaptureRef fCaptureDev;
   gl::TextureRef	fGlFrame;
   cv::Ptr<cv::CascadeClassifier> fClassifier;
   std::vector<cv::Rect> fDetections;
};

void CincerCvApp::setup()
{
   auto size = this->getWindowSize();
   
   // Initialize the camera.
   fCaptureDev = Capture::create(size[0], size[1]);
   fCaptureDev->start();
   
   // Set up detector with a HAAR cascade. See Resources folder for other types.
   auto path = cinder::app::Platform::get()->getResourcePath("haarcascade_eye.xml");
   fClassifier = new cv::CascadeClassifier(path.c_str());
   
   // Prepare a GPU buffer used to draw the captured frame.
   fGlFrame = gl::Texture::create(size.x, size.y);
}

void CincerCvApp::mouseDown( MouseEvent event )
{
}

void CincerCvApp::update()
{
   if (fCaptureDev->checkNewFrame())
   {
      // Capture the current frame.
      const Surface8uRef rgbFrame = fCaptureDev->getSurface();
      
      // Find the objects
      
      // Convert rgb -> gray -> cv::Mat
      cv::Mat ocvGrayImage = toOcv(Channel(*rgbFrame));
      fClassifier->detectMultiScale(ocvGrayImage, fDetections);
      
      // Get the frame in a format OpenGL can draw, i.e. load it to GPU.
      fGlFrame->update(*rgbFrame);
   }
}

void CincerCvApp::draw()
{
   gl::color(1, 1, 1); // restore white color or everything is tinted
   gl::draw(fGlFrame);
   
   for (const auto& detection : fDetections)
   {
      Rectf bounds(detection.tl().x,
                   detection.tl().y,
                   detection.br().x,
                   detection.br().y);
      
      // We'll move this around for our circles
      vec2 center = bounds.getCenter();
      
      // Whites
      gl::color(1, 1, 1);
      gl::drawSolidCircle(center, bounds.getWidth() / 2);
      
      // Pupil - black
      gl::color(0, 0, 0);
      center.y += bounds.getHeight() / 4;
      gl::drawSolidEllipse(center,bounds.getWidth() / 2, bounds.getHeight() / 2.5);
      
      // Highlight - white
      gl::color(1, 1, 1);
      center.y -= bounds.getHeight() / 4;
      center.x += bounds.getWidth() / 4;
      gl::drawSolidCircle(center, bounds.getWidth() / 8);
      
      // Outline - black
      gl::color(0, 0, 0);
      gl::drawStrokedCircle(bounds.getCenter(), bounds.getWidth() / 2);
      
      // Brow - black
      Rectf browFrame = bounds;
      float height = bounds.getHeight() / 4;
      browFrame.y1 = bounds.y1 - height * 1.15; // place above the eye with a gap
      browFrame.y2 = browFrame.y1 + height;
      browFrame.inflate(vec2(bounds.getWidth() * 0.075, 0));
      gl::drawSolidRect(browFrame);
   }
}

CINDER_APP( CincerCvApp, RendererGl )
