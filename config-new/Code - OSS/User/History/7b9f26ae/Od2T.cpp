#include <iostream>
#include "./firmwareUpdate.h"
#include <string>
#include <vector>
#include <sstream>
#include <curl/curl.h>

//how to run this in command line
//from within cpp folder
//g++ -std=c++11 main.cpp firmwareUpdate.cpp -lcurl && ./a.out

int main()
{
    FirmwareUpdater firmwareUpdater;
    std::cout << "main() -> Downloading files" << std::endl;
    firmwareUpdater.FirmwareFilesDownloadForVersionNumber("3.55", "/home/jimmy/Downloads/");
    std::cout << "main() -> Files done downloading" << std::endl;
    firmwareUpdater.FirmwareUpdateBegin(2, "3.55", "/home/jimmy/Downloads/");
    // response for this was  updateArray: 240 0 2 2 80 2 85 247


    int chip = 2;
    firmwareUpdater.FirmwareGetBlock({240, 0, 2, 2, 81, 0, 0, 0, 10, 104, 1, 2, 11, 247}, "3.55");
    //expected answer is 
    //[240,0,2,2,83,0,0,0,10,104,123,45,3,48,104,4,6,21,12,104,53,88,66,67,78,11,79,17,78,85,107,44,88,109,109,125,86,77,82,55,99,2,53,247]
    //[240,0,2,2,83,0,0,0,10,104,123,45,3,48,104,4,6,21,12,104,53,88,66,67,78,11,79,17,78,85,107,44,88,109,109,125,86,77,82,55,99,2,53,247]

    //std::cout << "main() -> Response: " << response << std::endl;
    return 0;
}