#include <Ws2tcpip.h>
#include <ao_comm/tcp_server.hpp>
#include <ao_comm_deps/AO/include/EthernetStandAlone.h>
#include <chrono>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

//_____________________________________________________________________________
class Receiver {
public:
  Receiver(const char *ip_addr, int port);
  ~Receiver();
  std::tuple<char *, int> ReceiveMessage();

private:
  int _recvSocket;
  int _sendSocket;

  std::tuple<char *, int> processCommand(char *message);
  void Close();
};

Receiver::Receiver(const char *ip_addr, int port) {
  // Windows only application
  // Initialize Winsock
  WSADATA wsaData;
  int wsaInitResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
  if (wsaInitResult != 0) {
    std::cerr << "WSAStartup failed: " << wsaInitResult << std::endl;
    return;
  }

  // Create receiver socket
  _recvSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (_recvSocket == INVALID_SOCKET) {
    std::cerr << "Error creating socket: " << WSAGetLastError() << std::endl;
    WSACleanup();
    return;
  }

  sockaddr_in recv_addr;
  recv_addr.sin_family = AF_INET;
  recv_addr.sin_port = htons(port);

  // Convert IP address from string to in_addr structure
  inet_pton(AF_INET, ip_addr, &recv_addr.sin_addr);

  // Bind to socket
  if (bind(_recvSocket, (SOCKADDR *)&recv_addr, sizeof(recv_addr)) ==
      SOCKET_ERROR) {
    std::cerr << "Bind failed: " << WSAGetLastError() << std::endl;
    closesocket(_recvSocket);
    WSACleanup();
    return;
  }

  // Listen on socket
  if (listen(_recvSocket, 1) == SOCKET_ERROR) {
    std::cerr << "Listen failed: " << WSAGetLastError() << std::endl;
    closesocket(_recvSocket);
    WSACleanup();
    return;
  }

  // Accept first sending client
  std::cout << "Waiting for incoming connection..." << std::endl;
  _sendSocket = accept(_recvSocket, NULL, NULL);
  if (_sendSocket == INVALID_SOCKET) {
    std::cerr << "Accept failed: " << WSAGetLastError() << std::endl;
    closesocket(_recvSocket);
    WSACleanup();
    return;
  }

  std::cout << "Connection established!" << std::endl;
}
Receiver::~Receiver() {
  closesocket(_sendSocket);
  closesocket(_recvSocket);
  WSACleanup();
}

//_____________________________________________________________________________
std::tuple<char *, int> Receiver::ReceiveMessage() {
  // Receive data
  char response[256];
  int dataSize;

  dataSize = recv(_sendSocket, response, sizeof(response), 0);

  // In case of -1 return, we did not receive a message
  if (dataSize == -1) {
    response[0] = 0;

  } else {
    response[dataSize] = 0; // do null termination
  }

  return processCommand(response);
}
void Receiver::Close() { closesocket(_recvSocket); }

//_____________________________________________________________________________
std::tuple<char *, int> Receiver::processCommand(char *message) {
  /*
   * Message must be in form of:
   *      FunctionName|<parameter1>|<parameter2>|<parameter3>|....
   *
   * The Following functions are available:
   *      STARTREC()
   *      STOPREC()
   *      STARTSTIM(
   *                              parameter1: StimChannel,
   *                              parameter2: FirstPhaseDelay_mS,
   *                              parameter3: FirstPhaseAmpl_mA,
   *                              parameter4: FirstPhaseWidth_mS,
   *                              parameter5: SecondPhaseDelay_mS,
   *                              parameter6: SecondPhaseAmpl_mA,
   *                              parameter7: SecondPhaseWidth_mS,
   *                              parameter8: Freq_hZ,
   *                              parameter9: Duration_sec,
   *                              parameter10: ReturnChannel
   *      )
   *      STOPSTIM(parameter1: StimChannel)
   *      SETPATH(parameter1: Path)
   *
   *      e.g.:
   *      STOPSTIM|10287
   */
  // If message is empty
  int errorCode = 0;
  if (strlen(message) == 0) {
    return std::make_tuple(message, errorCode);
  }
  // Make deep copy of message, because strtok function also changes char array
  // message :/
  size_t mLen = strlen(message) + 1;
  char *deepcopy_msg = new char[mLen];
  strcpy_s(deepcopy_msg, mLen, message);

  std::cout << "Received message: " << message << std::endl;

  // const char *delimiter = "|";

  // Split message by delimiter and store into list
  // char *msg_chars_array = strtok(message, delimiter);
  // std::vector<std::string> parameters;
  // while (msg_chars_array) {
  //   parameters.push_back(msg_chars_array);
  //   msg_chars_array = strtok(NULL, delimiter);
  // }
  char delimiter = '|';
  std::vector<std::string> parameters;

  // Try the above with stringstream instead
  std::stringstream sstr(
      message); // Create a stringstream from the input string
  std::string token;

  // Use std::getline with the custom delimiter
  while (std::getline(sstr, token, delimiter)) {
    parameters.push_back(token);
  }

  // Get first argument: Should be the name of the function to be called
  std::string functionName = parameters.at(0);

  // Printing for debug
  std::cout << "Received function name:" << functionName << std::endl;
  std::cout << "Received the following parameters" << std::endl;
  for (int i = 0; i < parameters.size(); i++) {
    std::cout << parameters[i] << std::endl;
  }
  std::cout << "Received the following parameters" << std::endl;

  // Call function on AO with corresponding parameters
  if (functionName == "STARTREC") {
    std::cout << "Calling function StartSave" << std::endl;
    errorCode = StartSave(); // start saving on AO
  } else if (functionName == "STOPREC") {
    std::cout << "Calling function StopSave" << std::endl;
    errorCode = StopSave(); // stop saving on AO
  } else if (functionName == "STARTSTIM") {
    // TODO: Implement proper json parson for incomming messages -> like this we
    // rely on the order
    try {
      int StimChannel = std::stoi(parameters.at(1));
      long double FirstPhaseDelay_mS = std::stold(parameters.at(2));
      double FirstPhaseAmpl_mA = std::stod(parameters.at(3));
      double FirstPhaseWidth_mS = std::stod(parameters.at(4));
      double SecondPhaseDelay_mS = std::stod(parameters.at(5));
      double SecondPhaseAmpl_mA = std::stod(parameters.at(6));
      double SecondPhaseWidth_mS = std::stod(parameters.at(7));
      double Freq_hZ = std::stod(parameters.at(8));
      double Duration_sec = std::stod(parameters.at(9));
      int ReturnChannel = std::stoi(parameters.at(10));

      std::cout << "Calling function StartDigitalStimulation with the "
                   "following parameters:"
                << std::endl;
      std::cout << "\tStimChannel: " << StimChannel << std::endl;
      std::cout << "\tFirstPhaseDelay_mS: " << FirstPhaseDelay_mS << std::endl;
      std::cout << "\tFirstPhaseAmpl_mA: " << FirstPhaseAmpl_mA << std::endl;
      std::cout << "\tFirstPhaseWidth_mS: " << FirstPhaseWidth_mS << std::endl;
      std::cout << "\tSecondPhaseDelay_mS: " << SecondPhaseDelay_mS
                << std::endl;
      std::cout << "\tSecondPhaseAmpl_mA: " << SecondPhaseAmpl_mA << std::endl;
      std::cout << "\tSecondPhaseWidth_mS: " << SecondPhaseWidth_mS
                << std::endl;
      std::cout << "\tFreq_hZ: " << Freq_hZ << std::endl;
      std::cout << "\tDuration_sec: " << Duration_sec << std::endl;
      std::cout << "\tReturnChannel: " << ReturnChannel << std::endl;
      errorCode = StartDigitalStimulation(
          StimChannel, FirstPhaseDelay_mS, FirstPhaseAmpl_mA,
          FirstPhaseWidth_mS, SecondPhaseDelay_mS, SecondPhaseAmpl_mA,
          SecondPhaseWidth_mS, Freq_hZ, Duration_sec, ReturnChannel);
    } catch (const std::out_of_range &e) {
      std::cout << "Not enough parameters were given for function: STARTSTIM"
                << std::endl;
      errorCode = -1;
    } catch (const std::invalid_argument &e) {
      std::cout << "Invalid parameter were given for function: STARTSTIM"
                << std::endl;
      std::cout << "Parameters should be given as: "
                << "<StimChannel: <int>, <FirstPhaseDelay_mS: <long double>, "
                   "<FirstPhaseAmpl_mA: double>, "
                << "<FirstPhaseWidth_mS: double>, <SecondPhaseDelay_mS: "
                   "double>, <SecondPhaseAmpl_mA: double>, "
                << "<SecondPhaseWidth_mS: double>, <Freq_hZ: double>, "
                   "<Duration_sec: double>, "
                << "<ReturnChannel: int>" << std::endl;
      errorCode = -1;
    }
  } else if (functionName == "STOPSTIM") {
    try {
      int StimChannel = std::stoi(parameters.at(1)); // Convert to int
      std::cout
          << "Calling function StopStimulation with the following parameter:"
          << std::endl;
      std::cout << "\tStimChannel: " << StimChannel << std::endl;
      errorCode = StopStimulation(StimChannel);
    } catch (const std::out_of_range &e) {
      std::cout << "Not enough parameters were given for function: STOPSTIM"
                << std::endl;
      errorCode = -1;
    } catch (const std::invalid_argument &e) {
      std::cout << "Invalid parameter were given for function: STOPSTIM"
                << std::endl;
      std::cout << "Parameter should be given as: "
                << "<StimChannel: int>" << std::endl;
      errorCode = -1;
    }
  } else if (functionName == "SETPATH") {
    char *path = &parameters.at(1)[0]; // Convert to char*

    std::cout << "Calling function SetSavePath with the following parameter:"
              << std::endl;
    std::cout << "\tPath: " << path << std::endl;
    errorCode = SetSavePath(path, 50);
  } else if (functionName == "SETSAVENAME") {
    char *path = &parameters.at(1)[0]; // Convert to char*

    std::cout
        << "Calling function SetSaveFileName with the following parameter:"
        << std::endl;
    std::cout << "\tFileName: " << path << std::endl;
    errorCode = SetSaveFileName(path, 30);
  } else {
    std::cout << "Function " << functionName << " is not available!"
              << std::endl;
    errorCode = -1;
  }
  return std::make_tuple(deepcopy_msg, errorCode);
}

void run_server(const char *ip_addr, int port, std::atomic_bool &stop_thread,
                int sleep_ms) {
  std::tuple<char *, int> msgErrorCodeTuple;
  std::string message;
  int errorCode;

  Receiver _messageReceiver = Receiver(ip_addr, port);

  while (!stop_thread) {
    // Reveice and execute function call on AO
    msgErrorCodeTuple = _messageReceiver.ReceiveMessage();
    message = std::get<0>(msgErrorCodeTuple);
    errorCode = std::get<1>(msgErrorCodeTuple);

    if (errorCode == 0 && !message.empty()) {
      std::cout << "Function call was successful" << std::endl;
    } else if (errorCode != 0 && !message.empty()) {
      std::cout << "Function call was not successful" << std::endl;
      std::cout << "Error code: " << errorCode << std::endl;
    }

    // Stop full speed running
    std::chrono::seconds dura(1);
    std::this_thread::sleep_for(dura);
  }
}
