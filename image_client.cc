/*
 *
 * Copyright 2015 gRPC authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>


#include <OpenImageIO/imageio.h>
#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>
#include "utils.h"


using namespace OIIO;
using namespace std;



#ifdef BAZEL_BUILD
#include "examples/protos/image.grpc.pb.h"
#else
#include "./image.grpc.pb.h"
#endif

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using image::NLImage;
using image::NLImageRotateRequest;
using image::NLImageService;


::image::NLImageRotateRequest_Rotation rotationStringToEnum(string str);


class ImageClient {
    
 public:
  
  /**
   * ImageClient  constructor
   *
   * @param channel Container whose values are summed.
   * @return ImageClient an actual client instance
   */
  ImageClient(std::shared_ptr<Channel> channel)
: stub_(NLImageService::NewStub(channel)) {}
  
  /**
   * SendProcessImage  Assembles the client's payload, sends it and presents the response back from the server.
   *
   * @param NLImage Source image
   * @param cmdLineOptions options for send from command line
   * @return string message to ourput
   */
  std::string SendProcessImage(NLImage* img, cmdLineOptions options) {
    
    // Data we are sending to the server.
    NLImageRotateRequest request;
    NLImage* imageToSend = img;

    request.set_rotation(rotationStringToEnum(options.rotate));
    request.set_allocated_image(imageToSend);


    // Container for the data we expect from the server.
    NLImage reply;
    NLImage filterReply;
    NLImage imgRequest;

    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    ClientContext context;

    // The actual RPC call for rotate.
    cout << "Calling RotateImage() rpc.." << endl;
    Status status = stub_->RotateImage(&context, request, &reply);

    // Act upon its status.
    if (status.ok()) {
      cout << "Bytes back from Rotate:" << reply.data().length() << endl;
      *img = reply;

    } else {
      std::cout << status.error_code() << ": " << status.error_message()
              << std::endl;
      return "RPC failed";
    }

    // The actual RPC call for mean.
    if (options.mean){
      
      ClientContext context2;
      cout << "Calling MeanFilter() rpc.." << endl;
      status = stub_->MeanFilter(&context2, reply, &filterReply);

      if (status.ok()){
        cout << "Bytes back from filter:" << filterReply.data().length() << endl;
        *img = reply;
        return "SUCCESS";
      } else {
        std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      return "RPC failed";
      }
    } else {
      return "SUCCESS";
    }

    return "SU?CCESS";
  }
  
  
  
  void writeReturnFile(NLImage returnImg){
  
    
    int imgWidth = returnImg.width();
    int imgHeight = returnImg.height();
    
    int numChannels = returnImg.color() ? 3: 1;
    
    
    cout << "width: " << imgWidth << " height:" << imgHeight <<endl;
    
    ImageSpec postagespec (imgWidth, imgHeight, numChannels, TypeDesc::FLOAT);
    ROI imgRoi;
    
    imgRoi.xbegin = 0;
    imgRoi.ybegin = 0;
    imgRoi.zbegin = 0;
    imgRoi.chbegin = 0;
    
    imgRoi.xend = imgWidth;
    imgRoi.yend = imgHeight;
    imgRoi.zend = 1;
    imgRoi.chend = numChannels;
    
    ImageBuf outBuf;
    
    // set the img buffer ready to operate on
    outBuf.reset(postagespec);
    //outBuf.make_writable(true);
    
    // set pixels
    char const *pixels = returnImg.data().c_str();
    bool pixelResult = outBuf.set_pixels(postagespec.roi(), TypeDesc::UINT8, pixels);
    
    pixelResult = outBuf.write("./returned.jpeg");
    
    cout << "wrote return file: " << pixelResult << endl;
  }
  
  void writeImageFile(NLImage *returnImg, string imgFilename) {
  
    
    int imgWidth = returnImg->width();
    int imgHeight = returnImg->height();
    
    int numChannels = returnImg->color() ? 3: 1;
    
    cout << "Writing output image.. width: " << imgWidth << " height:" << imgHeight <<endl;
    
    ImageSpec postagespec (imgWidth, imgHeight, numChannels, TypeDesc::FLOAT);
    ROI imgRoi;
    
    imgRoi.xbegin = 0;
    imgRoi.ybegin = 0;
    imgRoi.zbegin = 0;
    imgRoi.chbegin = 0;
    
    imgRoi.xend = imgWidth;
    imgRoi.yend = imgHeight;
    imgRoi.zend = 1;
    imgRoi.chend = numChannels;
    
    ImageBuf outBuf;
    
    // set the img buffer ready to operate on
    outBuf.reset(postagespec);
    //outBuf.make_writable(true);
    
    // set pixels
    char const *pixels = returnImg->data().c_str();
    bool pixelResult = outBuf.set_pixels(postagespec.roi(), TypeDesc::UINT8, pixels);
    
    pixelResult = outBuf.write(imgFilename);
    
  }

 private:
  std::unique_ptr<NLImageService::Stub> stub_;
};




::image::NLImageRotateRequest_Rotation rotationStringToEnum(string str){
  if (str == "NINETY_DEG") {
    return ::image::NLImageRotateRequest_Rotation_NINETY_DEG;
  } else if (str == "ONE_EIGHTY_DEG") {
    return ::image::NLImageRotateRequest_Rotation_ONE_EIGHTY_DEG;
  } else if ("TWO_SEVENTY_DEG") {
    return ::image::NLImageRotateRequest_Rotation_TWO_SEVENTY_DEG;
  } else {
    return ::image::NLImageRotateRequest_Rotation_NONE;
  }
}

bool getImageFromFile(string strFile, NLImage* image) {
  ImageBuf imgBuffer = ImageBuf(strFile);
  

  ImageSpec spec = imgBuffer.spec();
  ROI roi = spec.roi();
  
  int outImgWidth = spec.width;
  int outImgHeight = spec.height;
  int totalchannels = roi.nchannels();
  
  int size = outImgWidth * outImgHeight * totalchannels;
  unsigned char* a = new unsigned char[size];

  bool pixelResult = imgBuffer.get_pixels(roi, TypeDesc::UINT8, a);
  
  if (!pixelResult)
    return false;
  
  unsigned char* bytesToSend = (unsigned char*) a;
  int sizeToSend = size;

  bool bwrite = imgBuffer.write("./source.jpeg");
  cout << "wrote source: " << bwrite << endl;
  
  // *** try to copy pixels
  image->set_width(outImgWidth);
  image->set_height(outImgHeight);
  
  bool isColor = (totalchannels == 3) ? true: false;
  image->set_color(isColor);
  
  image->set_data(bytesToSend, sizeToSend);

  delete[] a;
  
  return true;
}




static const string DEFAULT_HOST = "localhost";
static const string DEFAULT_PORT = "50051";

int main(int argc, char** argv) {
  
  cmdLineOptions options;
  if (!parseCmdLine(argc, argv, options)){
    return -1;
  }

  // get end point
  string endpoint_host = (options.host == "") ? DEFAULT_HOST : options.host;
  string endpoint_port = (options.port == "") ? DEFAULT_PORT : options.port;
  string endpoint = options.host + ":" + options.port;


  NLImage* img = new NLImage();
  bool getResult = getImageFromFile(options.input, img);

  if (!getResult){
    cout << "Error loading image: " << options.input << endl;
    return -1;
  }

  grpc::ChannelArguments ch_args;
  ch_args.SetMaxReceiveMessageSize(-1);

  ImageClient imageClient(
  grpc::CreateCustomChannel(endpoint,
                            grpc::InsecureChannelCredentials(),
                            ch_args));

  std::string reply = imageClient.SendProcessImage(img, options);
  std::cout << "SendProcessImage: " << reply << std::endl;
  
  imageClient.writeImageFile(img, options.output);

  return 0;
}
