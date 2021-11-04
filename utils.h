/*
 *
 * Author: Miguel A. Alvarado
 *
 * Description: Interview problem, utility functions
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



#ifndef utils_h
#define utils_h

#include <fstream>

using namespace std;

struct cmdLineOptions {
  string host;
  string port;
  string rotate;
  bool mean = false;
  string input;
  string output;
};

/**
 * showUsage  show usage in cmd line -- this needs love, did the bare minimum
 *
 * @return void
 */
static void showUsage(){
  
  string client = "  ./client --port <...> --host <...> --input <...> --output <...> --rotate <...> --mean";
  string server = " ./server --port <...> --host <...>";
  
  
  cout << "Usage for client: " << endl;
  cout << "pepe" << endl;
  cout << endl;
  
  cout << "Usage for server: " << endl;
  cout << "pepe";
}



/**
 * parseCmdLine  Parse the cmd line -- we could have a much more elegant version, but this will be good for now
 * @param int fnumber of args
 * @param char** array of args
 * @param cmdLineOptions& options to return
 * @return bool does it seem to be valid
 */
static bool parseCmdLine(int argc, char** argv, cmdLineOptions& options) {
  if (argc < 3) {
    cout << "usage" << endl;
    return false;
  }
  
  std::vector <std::string> sources;
  std::string destination;
  
  for (int i = 0; i < argc; ++i) {
    std::string arg = argv[i];
    if ((arg == "-h") || (arg == "--help")) {
        showUsage();
        return true;
    } else if ((arg == "--host")) {
        if (i + 1 < argc) { // Make sure we aren't at the end of argv!
          options.host = argv[i+1]; // Increment 'i' so we don't get the argument as the next argv[i].
        } else { // Uh-oh, there was no argument to the destination option.
              std::cerr << "--host option requires one argument." << std::endl;
            return true;
        }
    } else if ((arg == "--port")) {
      if (i + 1 < argc) { // Make sure we aren't at the end of argv!
        options.port = argv[i+1]; // Increment 'i' so we don't get the argument as the next argv[i].
      } else { // Uh-oh, there was no argument to the destination option.
            std::cerr << "--port option requires one argument." << std::endl;
          return true;
      }
    } else if ((arg == "--rotate")) {
      if (i + 1 < argc) { // Make sure we aren't at the end of argv!
        options.rotate = argv[i+1]; // Increment 'i' so we don't get the argument as the next argv[i].
      } else { // Uh-oh, there was no argument to the destination option.
            std::cerr << "--rotate option requires one argument." << std::endl;
          return true;
      }
    } else if ((arg == "--input")) {
      if (i + 1 < argc) { // Make sure we aren't at the end of argv!
        options.input = argv[i+1]; // Increment 'i' so we don't get the argument as the next argv[i].
      } else { // Uh-oh, there was no argument to the destination option.
            std::cerr << "--input option requires one argument." << std::endl;
          return true;
      }
    } else if ((arg == "--output")) {
      if (i + 1 < argc) { // Make sure we aren't at the end of argv!
        options.output = argv[i+1]; // Increment 'i' so we don't get the argument as the next argv[i].
      } else { // Uh-oh, there was no argument to the destination option.
            std::cerr << "--output option requires one argument." << std::endl;
          return true;
      }
    } else if ((arg == "--mean")) {
      /*if (i + 1 < argc) { // Make sure we aren't at the end of argv!
        options.mean = argv[i+1]; // Increment 'i' so we don't get the argument as the next argv[i].
        
      } else { // Uh-oh,
            std::cerr << "--host option requires one argument." << std::endl;
          return true;
      }*/
      options.mean = true;
    }
  }
  
  cout << "Running with following options: " << endl;
  cout << "host: " << options.host << endl;
  cout << "port:" << options.port << endl;
  cout << "rotate:" << options.rotate << endl;
  cout << "mean:" << options.mean << endl;
  cout << "input:" << options.input << endl;
  cout << "output:" << options.output << endl;
  
  return true;
}



#endif /* utils_h */
