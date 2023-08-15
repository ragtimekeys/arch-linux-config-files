// how to run this in the command line
//$ g++ -std=c++11 main.cpp -lcurl && ./a.out

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <curl/curl.h>
#include "json.hpp"
#include "firmwareUpdate.h"
// namespace Creator
// {

FirmwareUpdater::FirmwareUpdater()
{
}

void FirmwareUpdater::sendMessage(const std::string &messageType, const std::string &messageInfo)
{
    std::cout << "CALLED: " << messageType << " ..... " << messageInfo << std::endl;
}

size_t WriteCallback(char *contents, size_t size, size_t nmemb, std::string *output)
{
    size_t totalSize = size * nmemb;
    output->append(contents, totalSize);
    return totalSize;
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

std::string fetch(const std::string &url)
{
    CURL *curl = curl_easy_init();
    if (curl)
    {
        std::string response;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK)
        {
            std::cerr << "Error while fetching URL: " << curl_easy_strerror(res) << std::endl;
        }
        curl_easy_cleanup(curl);
        return response;
    }
    return "";
}

void FirmwareUpdater::initiateFirmwareUpdate(int chip, const std::string &mostUpToDateFirmwareVersionFromServer)
{
    std::string savedMostUpToDateFirmwareVersionFromServer = mostUpToDateFirmwareVersionFromServer;
    std::cout << "CPP INITIATING FIRMWARE UPDATE" << std::endl;
    std::string chosenChipAsName = chip == 2 ? "dsp/" : "main/";
    std::string url = "https://linux.jamstik.com/files/firmware/generatedJsons/Studio/" + chosenChipAsName + mostUpToDateFirmwareVersionFromServer + ".json";
    std::cout << url << std::endl;

    std::string response = fetch(url);
    int lengthOfResponse = response.length();
    nlohmann::json jsonifiedFirmware = nlohmann::json::parse(response);
    m_firmwareFile = jsonifiedFirmware.get<std::vector<std::vector<int>>>();
    m_firmwareFile.back() = {0};

    int sizeOfFileToUpdate = m_firmwareFile.size() - 1;
    std::vector<int> updateArray{0xf0, 0x0, 0x2, 0x2, 0x50};
    std::vector<int> sizeOfFile = convertLine(sizeOfFileToUpdate);

    updateArray.insert(updateArray.end(), sizeOfFile.begin(), sizeOfFile.end());
    updateArray.push_back(chip);
    updateArray.insert(updateArray.end(), {0x55, 0xf7});

    // sendMessage("sendMidi", updat);
}

std::vector<std::string> fromCharCode(const std::vector<int>& charCodes) {
    std::vector<std::string> result;
    std::string str;
    for (int charCode : charCodes) {
        str += static_cast<char>(charCode);
    }
    result.push_back(str);
    return result;
}

std::string hexToBinary(const std::string& hexString) {
    std::string binaryString;
    for (char c : hexString) {
        switch (c) {
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

std::vector<std::string> hexToBinary(const std::vector<std::string>& hexStrings) {
    std::vector<std::string> binaryStrings;
    for (const std::string& hexString : hexStrings) {
        std::string binaryString = hexToBinary(hexString);
        binaryStrings.push_back(binaryString);
    }
    return binaryStrings;
}

std::string joinBinaryStrings(const std::vector<std::string>& binaryStrings) {
    std::string joinedString;
    for (const std::string& binaryString : binaryStrings) {
        joinedString += binaryString;
    }
    return joinedString;
}

std::vector<int> FirmwareUpdater::cyacdOrHexTo7bitBin(std::vector<int> &cyacdOrHexString)
{
    std::vector<std::string> finalBinArray;
    std::vector<int> finalBinArrayAsInts;
    std::string finalBinString;
    std::string numberToAdd;

    std::vector<std::string> charCodes = fromCharCode(cyacdOrHexString);

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

void printVector(const std::vector<int>& numbers) {
    std::cout << "Vector of Integers: [";
    for (size_t i = 0; i < numbers.size(); ++i) {
        std::cout << numbers[i];
        if (i < numbers.size() - 1) {
            std::cout << ", ";
        }
    }
    std::cout << "]" << std::endl;
}

std::vector<std::vector<int>> FirmwareUpdater::createLinesToSendBack(const std::vector<int> &lineJamstikGave, const std::string &typeOfJamstikRaw, int chip)
{

    int lineNumber = convertLine({lineJamstikGave[5],
                                  lineJamstikGave[6],
                                  lineJamstikGave[7],
                                  lineJamstikGave[8],
                                  lineJamstikGave[9]});

    if (lineNumber >= m_firmwareFile.size())
    {
        lineNumber = m_firmwareFile.size() - 1;
    }


    std::cout << "readSpecificLine called, line number: " << lineNumber << std::endl;

    int numberOfLines = lineJamstikGave[10];
    int hexOrCyacd;
    const int cyacd = 0x5b;
    const int hex = 0x53;

    hexOrCyacd = (chip == 2) ? hex : cyacd;

    std::vector<int> headerPart = {0xf0, 0x0, 0x2, 0x2, hexOrCyacd};
    std::vector<int> footerPart(2);

    std::vector<std::vector<int>> finalArrays;
    for (int i = 0; i < numberOfLines; i++)
    {
        std::vector<int> finalArray;
        std::vector<int> lineToInterpret = (lineNumber + i < m_firmwareFile.size()) ? m_firmwareFile[lineNumber + i] : std::vector<int>{0};
        std::cout << "Line is: ";
        printVector(lineToInterpret);
        std::vector<int> convertedLine = convertLine(lineNumber + i);
        finalArray.insert(finalArray.end(), headerPart.begin(), headerPart.end());
        finalArray.insert(finalArray.end(), convertedLine.begin(), convertedLine.end());

        std::stringstream ss;
        for (const auto &num : lineToInterpret)
        {
            ss << num << " ";
        }
        std::string lineToInterpretAsString = ss.str();

        if (hexOrCyacd == cyacd)
        {
            std::vector<int> cyacdBin = cyacdOrHexTo7bitBin(lineToInterpret);
            finalArray.insert(finalArray.end(), cyacdBin.begin(), cyacdBin.end());
        }
        else
        {
            std::vector<int> hexBin = cyacdOrHexTo7bitBin(lineToInterpret);
            finalArray.insert(finalArray.end(), hexBin.begin(), hexBin.end());
        }

        footerPart = {0x35, 0xf7};
        finalArray.insert(finalArray.end(), footerPart.begin(), footerPart.end());
        finalArrays.push_back(finalArray);
    }
    int percentageThrough = std::ceil(((lineNumber + numberOfLines) / static_cast<double>(m_firmwareFile.size())) * 100);

    // TODO: make sure to also send the percentage through thing
    return finalArrays;
}

void FirmwareUpdater::interpretUpdateSysex(const std::vector<int> &data, int chip, std::string savedMostUpToDateFirmwareVersionFromServer)
{
    if (data[4] == 0x54)
    {
        std::cout << "FINISHED" << std::endl;
        if (chip == 2)
        {
            chip = 1;
            initiateFirmwareUpdate(1, savedMostUpToDateFirmwareVersionFromServer);
        }
        else
        {
            sendMessage("firmwareUpdateFinished", "YAY");
        }
        return;
    }

    const std::vector<int> reservedUpdateCommandsSentByJamstik = {0x51, 0x54, 0x57, 0x5a};
    if (std::find(reservedUpdateCommandsSentByJamstik.begin(), reservedUpdateCommandsSentByJamstik.end(), data[4]) == reservedUpdateCommandsSentByJamstik.end())
    {
        return;
        // do nothing because this isn't jamstik update stuff
    }
    std::vector<std::vector<int>> dataToSend = createLinesToSendBack(data, "Studio", chip);

    for (size_t i = 0; i < dataToSend.size(); i++)
    {
        if (dataToSend[i].size() > 255 && false)
        {
            for (size_t j = 0; j < std::ceil(dataToSend[i].size() / 255.0); j++)
            {
                const std::vector<int> chunkToSend = std::vector<int>(dataToSend[i].begin() + j * 256, dataToSend[i].begin() + (j + 1) * 256);
                nlohmann::json jsonChunkToSend(chunkToSend);
                sendMessage("sendMidi", jsonChunkToSend.dump());
            }
        }
        else
        {
            nlohmann::json jsonDataToSend(dataToSend[0]);
            sendMessage("sendMidi", jsonDataToSend.dump());
        }
    }
}