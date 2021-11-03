//
//  utils.h
//  TestingGRPC
//
//  Created by Miguel Alvarado on 10/28/21.
//

#ifndef utils_h
#define utils_h


using namespace std;

struct cmdLineOptions {
  string host;
  string port;
  string rotate;
  bool mean = false;
  string input;
  string output;
};

static void showUsage(){
  
  string client = "  ./client --port <...> --host <...> --input <...> --output <...> --rotate <...> --mean";
  string server = " ./server --port <...> --host <...>";
  
  
  cout << "Usage for client: " << endl;
  cout << "pepe" << endl;
  cout << endl;
  
  cout << "Usage for server: " << endl;
  cout << "pepe";
}


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
