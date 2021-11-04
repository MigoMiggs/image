/*
 *
 * Author: Miguel A. Alvarado
 *
 * Description: Interview problem, server file.
 *
 * Notes: Added channels to the image definition so that PNGs with 4 channels work. There may be a bug with properly persisting
 * the alpha channel, skipping fix for the sake of time.
 *
 * There are some hacks for getting this done but could affect performance, ie: reading part if the image file via a stream, but
 * then reading again via the Open Image ImgBug API. Ideally only read once. Could also have done proper logging, but did not go there.
 * Tests would be critical in a real world scanario as well.
 *
 * This is my first time doing gRPC, protocol bugs and Open Image IO, so there are likely better ways to do many things. :)
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
  
  /**
   * RotateImage  Dispatch the rotate image RPC
   *
   * @param ServerContext gRPC context
   * @param NLImageRotateRequest Request data, including img
   * @param NLImage Img to returnd
   * @return Status
   */
  Status RotateImage(ServerContext* context, const NLImageRotateRequest* request,
                NLImage* reply) override {
  
    NLImage image = request->image();

    cout << "Running RotateImage() with bytes: " << image.data().length() << endl;
    

    return cmdRotate(request, reply);

  }
  
  /**
   * MeanFilter  Dispatch the filter image RPC
   *
   * @param ServerContext gRPC context
   * @param NLImageRotateRequest Request data, including img
   * @param NLImage Img to returnd
   * @return Status
   */
  Status MeanFilter(ServerContext* context, const NLImage* request,
                NLImage* reply) override {
  
    cout << "Running MeanFilter() with bytes:" << request->data().length() << endl;

    return cmdMeanFilter(request, reply);

  }
  
  /**
   * cmdMeanFilter  Do the axctial filter work... used the cmd name because originaly thought of a
   * command pattern which was overkill for this exercise
   *
   * @param NLImage Image in
   * @param NLImage Image out
   * @return Status
   */
  Status cmdMeanFilter(const NLImage* request, NLImage* reply){
    int imgWidth = request->width();
    int imgHeight = request->height();

    int numChannels = request->channels();
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

    cout << "Done mean!: " << endl;
    //pixelResult = outBuf.write("./processed_mean.jpeg"); <-- was using for debug


    return Status::OK;
  }
  
  /**
   * cmdRotate  Do the actual rotate work... used the cmd name because originaly thought of a
   * command pattern which was overkill for this exercise
   *
   * @param NLImage Image in
   * @param NLImage Image out
   * @return Status
   */
  Status cmdRotate(const NLImageRotateRequest* request, NLImage* reply){

    NLImage imageRequest = request->image();

    int imgWidth = imageRequest.width();
    int imgHeight = imageRequest.height();

    int numChannels = imageRequest.channels();
    int imgByteSize = imageRequest.data().length();

    // Need to set spec with img metadata
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

    // set pixels from the passed img
    char const *pixels = imageRequest.data().c_str();
    bool pixelResult = rotateBuf.set_pixels(postagespec.roi(), TypeDesc::UINT8, pixels);

    NLImageRotateRequest_Rotation rotation = request->rotation();

    if (!pixelResult)
      return Status::CANCELLED;

    
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
    reply->set_channels(numChannels);

    cout << "done rotate!: " << rotation << endl;
    // pixelResult = outBuf.write("./processed.jpeg"); <-- was using for debug
    
    return Status::OK;
  }
};


/**
 * RunServer  Start server
 *
 * @param string Host ip
 * @param string Port
 * @return Status
 */
void RunServer(string host, string port) {

  // could use validation on host and port, that can come later
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
    
  // make sure we don't run into size problems
  builder.SetMaxMessageSize(0x7FFFFFFF);
  builder.SetMaxMessageSize(-1);
  
  // Finally assemble the server.
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();
}

/**
 * main Entry point
 *
 */
int main(int argc, char** argv) {
  
  cmdLineOptions options;
  if (!parseCmdLine(argc, argv, options)){
    return -1;
  }
  
  RunServer(options.host, options.port);

  return 0;
}
