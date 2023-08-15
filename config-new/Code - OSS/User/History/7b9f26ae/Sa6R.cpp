#include <iostream>
#include "./firmwareUpdate.h"
#include <string>
#include <vector>
#include <sstream>
#include <curl/curl.h>

//how to run this in command line
//g++ -std=c++11 main.cpp firmwareUpdate.cpp -lcurl && ./a.out




int main()
{
    /*
    std::string url = "https://linux.jamstik.com/files/firmware/generatedJsons/Studio/dsp/3.55.json";
    std::string response = fetch(url);
    std::vector<int> example = convertLine(500);
    std::cout << "example " << example[0] << example[1] << example[2] << example[3] << example[4] << std::endl;
    */

//    std::string hexURL = "https://linux.jamstik.com/files/firmware/raw/Studio/dsp/3.55.hex";
//    std::string cyacdURL = "https://linux.jamstik.com/files/firmware/raw/Studio/main/3.55.cyacd";

//     std::string hexFileNameAndLocationToSave = "/Users/jimmy/3.55.hex";
//     std::string cyacdFileNameAndLocationToSave = "/Users/jimmy/3.55.cyacd";
//    if (saveFileFromURL(hexURL, hexFileNameAndLocationToSave)) {
//         std::cout << "Hex file downloaded and saved successfully." << std::endl;
//     }
    FirmwareUpdater firmwareUpdater;
    std::cout << "Downloading files" << std::endl;
    firmwareUpdater.downloadFirmwareFilesForVersionNumber("3.55", "/home/jimmy/Downloads/");
    std::cout << "Files done downloading" << std::endl;
    firmwareUpdater.FirmwareUpdateBegin(2, "3.55", "/home/jimmy/Downloads/");
    // response for this was  updateArray: 240 0 2 2 80 2 85 247

    //actual answer is
    //240,0,2,2,80,0,0,0,0,40,2,85,247

    // std::vector<int> hoo = {0,2,108,112,2};
    // int heee = convertLine(hoo);
    // std::cout << "hee is " << heee << std::endl;

    int chip = 2;
    firmwareUpdater.interpretUpdateSysex({240, 0, 2, 2, 81, 0, 0, 0, 10, 104, 1, 2, 11, 247}, 2, "3.55");
    //expected answer is 
    //[240,0,2,2,83,0,0,0,10,104,123,45,3,48,104,4,6,21,12,104,53,88,66,67,78,11,79,17,78,85,107,44,88,109,109,125,86,77,82,55,99,2,53,247]
    //[240,0,2,2,83,0,0,0,10,104,123,45,3,48,104,4,6,21,12,104,53,88,66,67,78,11,79,17,78,85,107,44,88,109,109,125,86,77,82,55,99,2,53,247]
    //but we got
    //[240,0,2,2,83,0,0,0,10,104,67,12,127,22,109,68,16,108,0,70,70,16,116,110,17,119,75,81,27,44,36,126,10,115,31,30,98,89,37,112,59,73,53,247]
    //[240,0,2,2,83,0,0,0,10,104,67,12,127,22,109,68,16,108,0,70,70,16,116,110,17,119,75,81,27,44,36,126,10,115,31,30,98,89,37,112,59,73,53,247]

    //std::cout << "Response: " << response << std::endl;
    return 0;
}