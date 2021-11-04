/*
 *
 * Author: Miguel A. Alvarado
 *
 * Description: Interview problem, client file.
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
  std::string SendProcessImage(NLImage* img, cmdLineOptions options, NLImage& returnImg) {
    
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
      returnImg = reply;

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
        writeImageFile(reply, "./returned.jpeg");
        returnImg = reply;
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
  
  

  
  /**
   * writeImageFile  Writes a NLImage objects to disk converting to a ImgBuf,
   *
   * @param NLImage Source image
   * @param String Output filename
   * @return void
   */
  void writeImageFile(NLImage &returnImg, string imgFilename) {
  
    
    int imgWidth = returnImg.width();
    int imgHeight = returnImg.height();
    
    int numChannels = returnImg.channels();
    
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
    char const *pixels = returnImg.data().c_str();
    bool pixelResult = outBuf.set_pixels(postagespec.roi(), TypeDesc::UINT8, pixels);
    
    pixelResult = outBuf.write(imgFilename);
    
  }

 private:
  std::unique_ptr<NLImageService::Stub> stub_;
};


/**
 * rotationStringToEnum  Utility function that turns a string into the enum.
 *
 * @param str Source image
 * @return NLImageRotateRequest_Rotation
 */
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

/**
 * getImageFromFile  Creates an NLImage object from a file
 *
 * @param string Source file
 * @param NLImage Image to return
 * @return bool
 */
bool getImageFromFile(string strFile, NLImage* image) {
  ImageBuf imgBuffer = ImageBuf(strFile);
  

  ImageSpec spec = imgBuffer.spec();
  ROI roi = spec.roi();
  
  int outImgWidth = spec.width;
  int outImgHeight = spec.height;
  int totalchannels = roi.nchannels();
  
  cout << "channels: " << totalchannels << endl;
  
  int size = outImgWidth * outImgHeight * totalchannels;
  unsigned char* a = new unsigned char[size];

  // *** try to copy pixels
  bool pixelResult = imgBuffer.get_pixels(roi, TypeDesc::UINT8, a);
  
  if (!pixelResult)
    return false;
  
  unsigned char* bytesToSend = (unsigned char*) a;
  int sizeToSend = size;

  bool bwrite = imgBuffer.write("./source.jpeg");
  cout << "wrote source: " << bwrite << endl;
 
  image->set_width(outImgWidth);
  image->set_height(outImgHeight);
  image->set_channels(totalchannels);
  
  bool isColor = (totalchannels > 3) ? true: false;
  image->set_color(isColor);
  image->set_data(bytesToSend, sizeToSend);

  delete[] a;
  
  return true;
}

static const unsigned char JPEG_FIRST_BYTE = 0xFF;
static const unsigned char PNG_FIRST_BYTE = 0x89;

/**
 * validJpgOrPng  Very light validation, checks the first byte for JPEG and PNG, this could check the entire header and beyond
 * @param string file name
 * @return bool does it seem to be valid
 */
static bool validJpgOrPng(string fileName){
 
  ifstream ifs(fileName);
  
  if (!ifs.good()){
    return false;
  }
  
  unsigned char firstbyte;
  
  string extension = fileName.substr(fileName.find_last_of(".") + 1);
  firstbyte = ifs.peek();
  cout << "valid: " << firstbyte << endl;
  
  if (extension == "jpeg" || extension == "JPEG"){
    if (firstbyte != JPEG_FIRST_BYTE)
      return false;
    
  } else if (extension == "png" || extension == ".PNG"){
    if (firstbyte != PNG_FIRST_BYTE)
      return false;
    
  } else {
    return false;
  }
  
  return true;

}


static const string DEFAULT_HOST = "localhost";
static const string DEFAULT_PORT = "50051";

/**
 * main Entry point
 *
 */
int main(int argc, char** argv) {
  
  cmdLineOptions options;
  if (!parseCmdLine(argc, argv, options)){
    return -1;
  }

  // get end point
  string endpoint_host = (options.host == "") ? DEFAULT_HOST : options.host;
  string endpoint_port = (options.port == "") ? DEFAULT_PORT : options.port;
  string endpoint = options.host + ":" + options.port;

  // make sure image is legit
  bool validImg = validJpgOrPng(options.input);
  
  if (!validImg){
    cout << "Invalid image file!!" << endl;
    return -1;
  }

  NLImage* img = new NLImage();
  bool getResult = getImageFromFile(options.input, img);

  if (!getResult){
    cout << "Error loading image: " << options.input << endl;
    return -1;
  }

  grpc::ChannelArguments ch_args;
  ch_args.SetMaxReceiveMessageSize(-1);
  
  NLImage outputImg;

  ImageClient imageClient(
  grpc::CreateCustomChannel(endpoint,
                            grpc::InsecureChannelCredentials(),
                            ch_args));

  std::string reply = imageClient.SendProcessImage(img, options, outputImg);
  std::cout << "SendProcessImage: " << reply << std::endl;
  
  imageClient.writeImageFile(outputImg, options.output);

  return 0;
}
