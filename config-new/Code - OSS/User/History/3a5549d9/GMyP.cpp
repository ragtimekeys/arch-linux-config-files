// how to run this in the command line
//$ g++ -std=c++11 main.cpp -lcurl && ./a.out

// rough communication process
/*
1. com->js FirmwareUpdateBegin (chip, "3.55")
2. js->give me line # x and also y number of lines after x
3. com->here is the lines that you asked for (with all the sysex boilerplate header and footer etc)
4. repeat step 2 and 3 many many times
5. If jamstik is done, it will say finished
6. computer decides whether or not it's *actually* finished by checking which chip was updated
7. if chip 2 was updated, still need to update 1

Creator UI -> user indicated they want to update FW
Creator -> download firmware images to tmp file
Creator -> FirmwareUpdater: install FW update (file)
Creator -> delete fw image

FW image -> getAppDataLocation() + "/tmp/fw.whatever"

nitty gritty binary
FirmwareUpdater <- JamstikHandler : whattosendtothejamstik,percentagefinishedsofar FirmwareGetBlock(whatthejamstiksenttothecomputer)

struct FirmwareGetBlockReturn {
    zx::Array<uint8_t> data;
    float percentFinished;
};




zx::Array<uint8_t> data;

data.alloc(size);
std::copy_n(vec.data(), data.data(), size);
jamstikHandler->sendFirmwareData(data);
*/

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <curl/curl.h>
#include "json.hpp"
#include "firmwareUpdate.h"
#include <fstream>
// namespace Creator
// {


FirmwareUpdater::FirmwareUpdater()
{
}

void FirmwareUpdater::FirmwareSendBlock(const std::string &messageType, const std::string &messageInfo)
{
    std::cout << "CALLED: " << messageType << " ..... " << messageInfo << std::endl;
}


int FirmwareUpdater::convertLine(const std::vector<int> &numberArray)
{
    int num1 = numberArray[0];
    int num2 = numberArray[1];
    int num3 = numberArray[2];
    int num4 = numberArray[3];
    int num5 = numberArray[4];
    int finalNumber = num5 / 8 + num4 * 16 + num3 * 2048 + num2 * 262114 + num1 * 335544332;
    return finalNumber;
}

// THIS WORKS
std::vector<int> FirmwareUpdater::convertLine(int number)
{
    std::vector<int> finalArray(5, 0);
    finalArray[0] = number / 4294967296;
    finalArray[1] = (number / 262144) % 2097152;
    finalArray[2] = (number / 2048) % 16384;
    finalArray[3] = (number / 16) % 128;
    finalArray[4] = (number % 16) * 8;
    return finalArray;
}

void FirmwareUpdater::addLineToFile(const std::string& filename, const std::string& content) {
    std::ofstream file(filename, std::ios_base::app);
    
    if (file.is_open()) {
        file << content;
        file.close();
        std::cout << "Line added to the file." << std::endl;
    }
    else {
        std::cout << "Unable to open file: " << filename << std::endl;
    }
}

size_t writeToFile(void* buffer, size_t size, size_t nmemb, void* userData) {
    std::ofstream* file = static_cast<std::ofstream*>(userData);
    file->write(static_cast<char*>(buffer), size * nmemb);
    return size * nmemb;
}

bool FirmwareUpdater::downloadFile(const std::string& url, const std::string& outputPath) {
    CURL* curl = curl_easy_init();
    if (curl) {
        std::ofstream outputFile(outputPath, std::ios::binary);
        if (outputFile.is_open()) {
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeToFile);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &outputFile);
            CURLcode res = curl_easy_perform(curl);
            if (res != CURLE_OK) {
                std::cerr << "Failed to download file: " << curl_easy_strerror(res) << std::endl;
                return false;
            }
            curl_easy_cleanup(curl);
            outputFile.close();
            std::cout << "File downloaded successfully." << std::endl;
            return true;
        }
        else {
            std::cerr << "Failed to open output file." << std::endl;
        }
    }
    else {
        std::cerr << "Failed to initialize libcurl." << std::endl;
    }
    return false;
}

// saves both hex and cyacd files for a given firmware version
void FirmwareUpdater::FirmwareFilesDownloadForVersionNumber(std::string versionNumber, std::string directoryToSaveFiles)
{
    for (int chip = 1; chip <= 2; chip++)
    {
        std::string chosenChipAsName = chip == 2 ? "dsp/" : "main/";
        std::string fileNameAndPath = directoryToSaveFiles + versionNumber + (chip == 2 ? ".hex" : ".cyacd");
        std::string url = "https://linux.jamstik.com/files/firmware/raw/Studio/" + chosenChipAsName + versionNumber + "." + (chip == 2 ? "hex" : "cyacd");
        std::cout << "url is: " << url << std::endl;
        if (downloadFile(url, fileNameAndPath))
        {
            //add empty 0 to the end
            //addLineToFile(fileNameAndPath, ":0");
            std::cout << "File downloaded and saved successfully." << std::endl;
        }
    }
}


int FirmwareUpdater::countLines(const std::string& filename) {
    std::ifstream file(filename);
    std::string line;
    int lineCount = 0;

    if (file.is_open()) {
        while (std::getline(file, line)) {
            lineCount++;
        }

        file.close();
    }
    else {
        std::cout << "Unable to open file: " << filename << std::endl;
    }

    return lineCount;
}

std::string FirmwareUpdater::convertVectorToString(const std::vector<int>& numbers) {
    std::stringstream ss;
    
    for (size_t i = 0; i < numbers.size(); ++i) {
        ss << numbers[i];
        
        // Append a comma for all elements except the last one
        if (i < numbers.size() - 1) {
            ss << ",";
        }
    }
    
    return ss.str();
}


void FirmwareUpdater::FirmwareUpdateBegin(int chip, const std::string &mostUpToDateFirmwareVersionFromServer, const std::string &directoryForFirmwareFiles)
{
    //passing in chip is redundant, but it's nice to make sure that it starts at chip 2
    m_chip = chip;
    std::string savedMostUpToDateFirmwareVersionFromServer = mostUpToDateFirmwareVersionFromServer;
    std::cout << "CPP INITIATING FIRMWARE UPDATE" << std::endl;
    std::string chosenChipAsName = m_chip == 2 ? "dsp/" : "main/";
    std::string url = "https://linux.jamstik.com/files/firmware/raw/Studio/" + chosenChipAsName + mostUpToDateFirmwareVersionFromServer + ".json";
    std::cout << url << std::endl;
    std::string fullPathAndFileNameForHexChip2 = directoryForFirmwareFiles + mostUpToDateFirmwareVersionFromServer + ".hex";
    std::string fullPathAndFileNameForCyacdChip1 = directoryForFirmwareFiles + mostUpToDateFirmwareVersionFromServer + ".cyacd";
    //appdata/creator/firmware/3.34.cyacd
    //appdata/creator/firmware/3.45.cyacd
    //appdata/creator/firmware/3.55.cyacd ~3.55

    m_chip1FullPath = fullPathAndFileNameForCyacdChip1;
    m_chip2FullPath = fullPathAndFileNameForHexChip2;
    m_selectedFirmwareFileFullPath = m_chip == 2 ? m_chip2FullPath : m_chip1FullPath;

    m_firmwareFile.open(m_chip == 2 ? fullPathAndFileNameForHexChip2 : fullPathAndFileNameForCyacdChip1);


    std::cout << "full path chip 2: " << fullPathAndFileNameForHexChip2 << std::endl;
    std::cout << "full path chip 1: " << fullPathAndFileNameForCyacdChip1 << std::endl;
    if (m_chip == 2) {
        m_numberOfLinesInHexFirmwareFile = countLines(fullPathAndFileNameForHexChip2);
    } else /*basically if m_chip == 1*/ {
        m_numberOfLinesInCyacdFirmwareFile = countLines(fullPathAndFileNameForCyacdChip1);
    }
    int sizeOfFileToUpdate = m_chip == 2 ? m_numberOfLinesInHexFirmwareFile : m_numberOfLinesInCyacdFirmwareFile;

    std::vector<int> updateArray{0xf0, 0x0, 0x2, 0x2, 0x50};
    std::vector<int> sizeOfFile = convertLine(sizeOfFileToUpdate);

    updateArray.insert(updateArray.end(), sizeOfFile.begin(), sizeOfFile.end());
    updateArray.push_back(m_chip);
    updateArray.insert(updateArray.end(), {0x55, 0xf7});
    // nlohmann::json jsonTest = updateArray;
    FirmwareUpdater::FirmwareSendBlock("sendMidi", convertVectorToString(updateArray));
}

std::vector<std::string> FirmwareUpdater::fromCharCode(const std::vector<int> &charCodes)
{
    std::vector<std::string> result;
    std::string str;
    for (int charCode : charCodes)
    {
        str += static_cast<char>(charCode);
    }
    result.push_back(str);
    return result;
}

std::string FirmwareUpdater::hexToBinary(const std::string &hexString)
{
    // const std::vector<int>& charCodes
    // for (int cInt : charCodes)
    // {
    //    char c = static_cast<char>(cInt);
    //    switch (c) {}
    // }
    std::string binaryString;
    for (char c : hexString)
    {
        switch (c)
        {
        case '0':
            binaryString += "0000";
            break;
        case '1':
            binaryString += "0001";
            break;
        case '2':
            binaryString += "0010";
            break;
        case '3':
            binaryString += "0011";
            break;
        case '4':
            binaryString += "0100";
            break;
        case '5':
            binaryString += "0101";
            break;
        case '6':
            binaryString += "0110";
            break;
        case '7':
            binaryString += "0111";
            break;
        case '8':
            binaryString += "1000";
            break;
        case '9':
            binaryString += "1001";
            break;
        case 'A':
        case 'a':
            binaryString += "1010";
            break;
        case 'B':
        case 'b':
            binaryString += "1011";
            break;
        case 'C':
        case 'c':
            binaryString += "1100";
            break;
        case 'D':
        case 'd':
            binaryString += "1101";
            break;
        case 'E':
        case 'e':
            binaryString += "1110";
            break;
        case 'F':
        case 'f':
            binaryString += "1111";
            break;
        default:
            // Invalid hexadecimal character
            break;
        }
    }
    return binaryString;
}

std::vector<std::string> FirmwareUpdater::hexToBinary(const std::vector<std::string> &hexStrings)
{
    std::vector<std::string> binaryStrings;
    for (const std::string &hexString : hexStrings)
    {
        std::string binaryString = hexToBinary(hexString);
        binaryStrings.push_back(binaryString);
    }
    return binaryStrings;
}

std::string FirmwareUpdater::joinBinaryStrings(const std::vector<std::string> &binaryStrings)
{
    std::string joinedString;
    for (const std::string &binaryString : binaryStrings)
    {
        joinedString += binaryString;
    }
    return joinedString;
}

std::vector<std::string> FirmwareUpdater::splitString(const std::string& inputString) {
    std::vector<std::string> result;

    for (char c : inputString) {
        result.push_back(std::string(1, c));
    }

    return result;
}

std::vector<int> FirmwareUpdater::cyacdOrHexTo7bitBin(std::string cyacdOrHexString)
{
    std::vector<std::string> finalBinArray;
    std::vector<int> finalBinArrayAsInts;
    std::string finalBinString;
    std::string numberToAdd;

    std::vector<std::string> charCodes = splitString(cyacdOrHexString);

    //  CONVERT TO BINARY STRING
    finalBinString = joinBinaryStrings(hexToBinary(charCodes));

    //  SPLIT BY 7 BITS
    size_t startPos = 0;
    while (startPos < finalBinString.length())
    {
        finalBinArray.push_back(finalBinString.substr(startPos, 7));
        startPos += 7;
    }

    //  MAKES SURE LAST DIGIT IS 7 BITS LONG BY ADDING ZEROS
    while (finalBinArray[finalBinArray.size() - 1].length() < 7)
    {
        finalBinArray[finalBinArray.size() - 1] = finalBinArray[finalBinArray.size() - 1] + "0";
    }

    //  CONVERT 7 BIT BINARY STRING THINGS TO ACTUAL DECIMAL NUMBERS
    for (size_t i = 0; i < finalBinArray.size(); i++)
    {
        finalBinArrayAsInts.push_back(std::stoi(finalBinArray[i], nullptr, 2));
    }

    return finalBinArrayAsInts;
}


/*

input: lineJamstikGave [240, 2,2,2, ,34,34,23454, GIVEMELINE40, 242343,234354,545];
output: ok here is line 40: [240, 2,2, 2,234, line40goeshere, 340,340,]

*/

std::string FirmwareUpdater::readSpecificLine(const std::string& filename, int lineNumber) {
    std::cout << "readSpecificLine called, filename: " << filename << ", line number: " << lineNumber << std::endl;
    std::ifstream inputFile(filename);
    std::string line;
    int currentLine = 0;

    // Read lines until the desired line or end of file is reached
    while (std::getline(inputFile, line) && currentLine < lineNumber) {
        currentLine++;
    }

    // Return the line if the desired line is found
    if (currentLine == lineNumber) {
        return line;
    } else {
        return "Line not found";
    }
}

std::vector<std::vector<int>> FirmwareUpdater::createLinesToSendBack(const std::vector<int> &lineJamstikGave, const std::string &typeOfJamstikRaw)
{

    //40
    int lineNumber = convertLine({lineJamstikGave[5],
                                  lineJamstikGave[6],
                                  lineJamstikGave[7],
                                  lineJamstikGave[8],
                                  lineJamstikGave[9]});

    // if (lineNumber >= m_firmwareFile.size())
    // {
    //     lineNumber = m_firmwareFile.size() - 1;
    // }

    int numberOfLines = lineJamstikGave[10];
    int hexOrCyacd;
    const int cyacd = 0x5b;
    const int hex = 0x53;

    hexOrCyacd = (m_chip == 2) ? hex : cyacd;

    std::vector<int> headerPart = {0xf0, 0x0, 0x2, 0x2, hexOrCyacd};
    std::vector<int> footerPart(2);

    std::vector<std::vector<int>> finalArrays;
    int numOfLinesForCurrentFirmwareFile = m_chip == 2 ? m_numberOfLinesInHexFirmwareFile : m_numberOfLinesInCyacdFirmwareFile;
    for (int i = 0; i < numberOfLines; i++)
    {
        std::vector<int> finalArray;
        std::string line = readSpecificLine(m_selectedFirmwareFileFullPath, lineNumber);
        //std::getline(m_firmwareFile, line);
        m_currentLineNumber++;
        size_t colonPos = line.find(':');
        while (colonPos != std::string::npos)
        {
            line.erase(colonPos, 1);
            colonPos = line.find(':', colonPos);
        }

        std::cout << line << std::endl;
        std::string lineToInterpret = (lineNumber + i < numOfLinesForCurrentFirmwareFile) ? line : "0";
        std::vector<int> convertedLine = convertLine(lineNumber + i);
        finalArray.insert(finalArray.end(), headerPart.begin(), headerPart.end());
        finalArray.insert(finalArray.end(), convertedLine.begin(), convertedLine.end());

        // std::stringstream ss;
        // for (const auto &num : lineToInterpret)
        // {
        //     ss << num << " ";
        // }
        // std::string lineToInterpretAsString = ss.str();

        std::vector<int> convertedStuff = cyacdOrHexTo7bitBin(lineToInterpret);
        finalArray.insert(finalArray.end(), convertedStuff.begin(), convertedStuff.end());

        footerPart = {0x35, 0xf7};
        finalArray.insert(finalArray.end(), footerPart.begin(), footerPart.end());
        finalArrays.push_back(finalArray);
    }
    int percentageThrough = std::ceil(((lineNumber + numberOfLines) / static_cast<double>(numOfLinesForCurrentFirmwareFile)) * 100);

    // TODO: make sure to also send the percentage through thing
    return finalArrays;
}

void FirmwareUpdater::FirmwareUpdateComplete() {
    std::cout << "FirmwareUpdateComplete()" << std::endl;
}
void FirmwareUpdater::FirmwareUpdateCancel() {
    //need to make sure we are on DSP chip, if we're on main chip, you shouldn't be allowed to cancel an update

}

void FirmwareUpdater::FirmwareGetBlock(const std::vector<int> &data, std::string savedMostUpToDateFirmwareVersionFromServer)
{
    std::cout << "FirmwareGetBlock called" << std::endl;
    if (data[4] == 0x54)
    {
        FirmwareUpdater::FirmwareUpdateComplete();
        if (m_chip == 2)
        {
            m_chip = 1;
            FirmwareUpdateBegin(1, savedMostUpToDateFirmwareVersionFromServer, "/Users/jimmy/");
        }
        else
        {
            FirmwareUpdater::FirmwareSendBlock("firmwareUpdateFinished", "YAY");
        }
        return;
    }

    const std::vector<int> reservedUpdateCommandsSentByJamstik = {0x51, 0x54, 0x57, 0x5a};
    if (std::find(reservedUpdateCommandsSentByJamstik.begin(), reservedUpdateCommandsSentByJamstik.end(), data[4]) == reservedUpdateCommandsSentByJamstik.end())
    {
        return;
        // do nothing because this isn't jamstik update stuff
    }
    std::vector<std::vector<int>> dataToSend = createLinesToSendBack(data, "Studio");

    for (size_t i = 0; i < dataToSend.size(); i++)
    {
        // might not need to do this
        if (dataToSend[i].size() > 255 && false)
        {
            for (size_t j = 0; j < std::ceil(dataToSend[i].size() / 255.0); j++)
            {
                const std::vector<int> chunkToSend = std::vector<int>(dataToSend[i].begin() + j * 256, dataToSend[i].begin() + (j + 1) * 256);
                nlohmann::json jsonChunkToSend(chunkToSend);
                FirmwareUpdater::FirmwareSendBlock("sendMidi", jsonChunkToSend.dump());
            }
        }
        else
        {
            nlohmann::json jsonDataToSend(dataToSend[0]);
            FirmwareUpdater::FirmwareSendBlock("sendMidi", jsonDataToSend.dump());
        }
    }
}