//how to run this in command line
//from within cpp folder
//g++ -std=c++11 main.cpp firmwareUpdate.cpp -lcurl && ./a.out

#include <fstream>
#include <iostream>
#include <vector>

class FirmwareUpdater
{
public:
    FirmwareUpdater();

    //to be called from external source
    void FirmwareFilesDownloadForVersionNumber(std::string versionNumber, std::string directoryToSaveFiles);
    void FirmwareUpdateBegin(int chip, const std::string &mostUpToDateFirmwareVersionFromServer, const std::string &directoryForFirmwareFiles);
    void FirmwareGetBlock(const std::vector<int> &data, std::string savedMostUpToDateFirmwareVersionFromServer);
    void FirmwareUpdateCancel(); //TODO: this may have a callback from jamstik
    
    //to be triggered based on interal class code
    void FirmwareSendBlock(const std::string &messageType, const std::string &messageInfo);  //messageType is just placeholder
    void FirmwareUpdateComplete();
    void FirmwareUpdateAlert(bool finished); //if finished == 0, this means it started. maybe this is confusing?


private:
    std::ifstream m_firmwareFile;
    int m_chip = 2; //starts with DSP
    int m_currentLineNumber = 0;
    int m_numberOfLinesInCyacdFirmwareFile;
    int m_numberOfLinesInHexFirmwareFile;

    std::string m_selectedFirmwareFileFullPath;
    std::string m_chip1FullPath;
    std::string m_chip2FullPath;

    std::vector<std::string> splitString(const std::string& inputString);
    std::string joinBinaryStrings(const std::vector<std::string> &binaryStrings);
    std::vector<std::string> hexToBinary(const std::vector<std::string> &hexStrings);
    std::string hexToBinary(const std::string &hexString);
    std::vector<std::string> fromCharCode(const std::vector<int> &charCodes);
    std::string convertVectorToString(const std::vector<int>& numbers);
    int countLines(const std::string& filename);
    //size_t writeToFile(void* buffer, size_t size, size_t nmemb, void* userData);
    void addLineToFile(const std::string& filename, const std::string& content);
    bool downloadFile(const std::string& url, const std::string& outputPath);
    std::string readSpecificLine(const std::string& filename, int lineNumber); 
    int convertLine(const std::vector<int> &numberArray);
    std::vector<int> convertLine(int number);
    std::vector<int> cyacdOrHexTo7bitBin(std::string cyacdOrHexString);
    std::vector<std::vector<int>> createLinesToSendBack(const std::vector<int> &lineJamstikGave, const std::string &typeOfJamstikRaw);
};
