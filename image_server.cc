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

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

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



using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using image::NLImage;
using image::NLImageRotateRequest;
using image::NLImageRotateRequest_Rotation;
using image::NLImageService;



// Logic and data behind the server's behavior.
class NLImageServiceImpl final : public NLImageService::Service {
  
  Status RotateImage(ServerContext* context, const NLImageRotateRequest* request,
                NLImage* reply) override {
  
    NLImage image = request->image();

    cout << "Running RotateImage() with bytes: " << image.data().length() << endl;
    

    return cmdRotate(request, reply);

  }
  
  Status MeanFilter(ServerContext* context, const NLImage* request,
                NLImage* reply) override {
  
    cout << "we got MEAN " << request->data().length() << endl;
    cout << "Width: " << request->width() << " Height: " << request->height() << endl;

    return cmdMeanFilter(request, reply);

  }
  
  Status cmdMeanFilter(const NLImage* request, NLImage* reply){
    int imgWidth = request->width();
    int imgHeight = request->height();

    int numChannels = request->color() ? 3: 1;
    int imgByteSize = request->data().length();

    ImageSpec postagespec (imgWidth, imgHeight, numChannels, TypeDesc::UINT8);
    ROI imgRoi;

    imgRoi.xbegin = 0;
    imgRoi.ybegin = 0;
    imgRoi.zbegin = 0;
    imgRoi.chbegin = 0;

    imgRoi.xend = imgWidth;
    imgRoi.yend = imgHeight;
    imgRoi.zend = 1;
    imgRoi.chend = numChannels;

    ImageBuf meanBuf;
    ImageBuf outBuf;

    // set the img buffer ready to operate on
    meanBuf.reset(postagespec);

    // set pixels
    char const *pixels = request->data().c_str();
    bool pixelResult = meanBuf.set_pixels(postagespec.roi(), TypeDesc::UINT8, pixels);

    outBuf = ImageBufAlgo::median_filter (meanBuf, 3, 3);

    char* a = new char[imgByteSize];

    pixelResult = outBuf.get_pixels(postagespec.roi(), TypeDesc::UINT8, a);

    if (!pixelResult)
      return Status::CANCELLED;

    reply->set_data(a, imgByteSize);
    reply->set_width(imgWidth);
    reply->set_height(imgHeight);
    reply->set_color(request->color());

    cout << "done mean!: " << endl;
    pixelResult = outBuf.write("./processed_mean.jpeg");

    cout << "We are here!! " << pixelResult << endl;

    return Status::OK;
  }
  
  Status cmdRotate(const NLImageRotateRequest* request, NLImage* reply){
    cout << "let's get to work here" << endl;

    NLImage imageRequest = request->image();

    int imgWidth = imageRequest.width();
    int imgHeight = imageRequest.height();

    int numChannels = imageRequest.color() ? 3: 1;
    int imgByteSize = imageRequest.data().length();

    ImageSpec postagespec (imgWidth, imgHeight, numChannels, TypeDesc::UINT8);
    ROI imgRoi;

    imgRoi.xbegin = 0;
    imgRoi.ybegin = 0;
    imgRoi.zbegin = 0;
    imgRoi.chbegin = 0;

    imgRoi.xend = imgWidth;
    imgRoi.yend = imgHeight;
    imgRoi.zend = 1;
    imgRoi.chend = numChannels;

    ImageBuf rotateBuf;
    ImageBuf outBuf;

    // set the img buffer ready to operate on
    rotateBuf.reset(postagespec);

    // set pixels
    char const *pixels = imageRequest.data().c_str();
    bool pixelResult = rotateBuf.set_pixels(postagespec.roi(), TypeDesc::UINT8, pixels);

    NLImageRotateRequest_Rotation rotation = request->rotation();

    if (!pixelResult)
      return Status::CANCELLED;

    cout << "about to rotate with: " << rotation << endl;
    
    // Do the rotation
    if (rotation == image::NLImageRotateRequest_Rotation_NONE){
      outBuf = rotateBuf;
    } else {
      switch (rotation) {
      case image::NLImageRotateRequest_Rotation_NINETY_DEG:
        outBuf = ImageBufAlgo::rotate90 (rotateBuf);
        break;
        
      case image::NLImageRotateRequest_Rotation_ONE_EIGHTY_DEG:
        outBuf = ImageBufAlgo::rotate180 (rotateBuf);
        break;
        
      case image::NLImageRotateRequest_Rotation_TWO_SEVENTY_DEG:
        outBuf = ImageBufAlgo::rotate270 (rotateBuf);
        break;
        
        
      default:
        cout << "rotate not found: " << rotation << endl;
        return Status::CANCELLED;
        break;
      }
    }

    char* a = new char[imgByteSize];

    pixelResult = outBuf.get_pixels(postagespec.roi(), TypeDesc::UINT8, a);

    reply->set_data(a, imgByteSize);
    reply->set_width(imgWidth);
    reply->set_height(imgHeight);
    reply->set_color(imageRequest.color());

    cout << "done rot!: " << rotation << endl;
    pixelResult = outBuf.write("./processed.jpeg");

    cout << "We are here!! " << pixelResult << endl;
    
    return Status::OK;
  }
};



void RunServer(string host, string port) {

  std::string server_address(host + ":" + port);
    
  NLImageServiceImpl service;

  grpc::EnableDefaultHealthCheckService(true);
  grpc::reflection::InitProtoReflectionServerBuilderPlugin();
  ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&service);
    
    
  builder.SetMaxMessageSize(0x7FFFFFFF);
  builder.SetMaxMessageSize(-1);


  // Finally assemble the server.
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();
}

int main(int argc, char** argv) {
  
  cmdLineOptions options;
  if (!parseCmdLine(argc, argv, options)){
    return -1;
  }
  
  RunServer(options.host, options.port);

  return 0;
}
