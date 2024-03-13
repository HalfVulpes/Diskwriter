#include <iostream>
#include <windows.h>
#include <ctime>
#include <random>
#include <vector>
#include <chrono>
#include <iomanip>
#include <fstream>
#include <thread>
#include <atomic>
#include <string>
#include <sstream>
#include <map>
#include <locale>
#include <codecvt>
#include <mutex>

using namespace std;

atomic<bool> continuePrinting(true);
atomic<size_t> totalWrites(0);
chrono::time_point<chrono::high_resolution_clock> startTime;

string formatTime(long long seconds) {
    stringstream ss;
    ss << setw(2) << setfill('0') << seconds / 3600 << ":"
        << setw(2) << setfill('0') << (seconds % 3600) / 60 << ":"
        << setw(2) << setfill('0') << seconds % 60;
    return ss.str();
}

void printStatus() {
    cout << setw(15) << "     " << setw(15) << "Total Writes" << setw(15) << "Cur. Speed"
        << "          " << "Avg. Speed" << setw(15) << "Time Used" << endl;

    double avgSpeed = 0.0;
    size_t sampleCount = 0;
    while (continuePrinting) {
        auto currentTime = chrono::high_resolution_clock::now();
        auto elapsedTime = chrono::duration_cast<chrono::seconds>(currentTime - startTime).count();
        double currentSpeed = static_cast<double>(totalWrites) / 1024 / 1024 / (elapsedTime > 0 ? elapsedTime : 1);

        if (sampleCount > 0) {
            avgSpeed = avgSpeed * (1 - 1.0 / sampleCount) + (1.0 / sampleCount) * currentSpeed;
        }
        else {
            avgSpeed = currentSpeed;
        }
        sampleCount++;

        cout << fixed << setprecision(2);
        cout << "\r" << setw(7) << "Status"
            << setw(15) << totalWrites / 1024 / 1024 << " MB"
            << setw(15) << currentSpeed << " MB/s"
            << setw(15) << avgSpeed << " MB/s"
            << setw(15) << formatTime(elapsedTime) << flush;

        this_thread::sleep_for(chrono::seconds(1)); // Every second
    }
}


void listDisks() {
    DWORD drives = GetLogicalDrives();
    wcout << L"Local drive list (removable ones marked with \"*\")" << endl;

    for (wchar_t drive = L'A'; drive <= L'Z'; drive++) {
        if (drives & 1) {
            wstring drivePath = wstring(1, drive) + L":\\";
            UINT driveType = GetDriveType(drivePath.c_str());

            wstring driveTypeStr;
            if (driveType == DRIVE_REMOVABLE) {
                driveTypeStr = L"*";
            }

            WCHAR volumeNameBuffer[100] = { 0 };
            WCHAR fileSystemNameBuffer[10] = { 0 };
            DWORD serialNumber = 0, maxComponentLen = 0, fileSystemFlags = 0;

            GetVolumeInformation(
                drivePath.c_str(),
                volumeNameBuffer,
                sizeof(volumeNameBuffer) / sizeof(WCHAR),
                &serialNumber,
                &maxComponentLen,
                &fileSystemFlags,
                fileSystemNameBuffer,
                sizeof(fileSystemNameBuffer) / sizeof(WCHAR)
            );

            ULARGE_INTEGER freeBytesAvailable, totalNumberOfBytes, totalNumberOfFreeBytes;
            GetDiskFreeSpaceEx(drivePath.c_str(), &freeBytesAvailable, &totalNumberOfBytes, &totalNumberOfFreeBytes);

            wstring volumeName = wcslen(volumeNameBuffer) > 0 ? volumeNameBuffer : L"DISK";
            wcout << L"  [" << drive << L":]" << driveTypeStr << L" "
                << setw(10) << left << volumeName << L"  "
                << setw(10) << serialNumber << L"  "
                << totalNumberOfBytes.QuadPart / (1024 * 1024 * 1024) << L" GB" << endl;
        }
        drives >>= 1;
    }

    wcout << L"\nInput drive letter (letter ONLY, press Enter to continue, Ctrl-C to exit): ";
}

std::mutex mtx;
bool deleteFilesInDirectory(const std::wstring& directoryPath) {
    WIN32_FIND_DATA findFileData;
    HANDLE hFind = FindFirstFile((directoryPath + L"\\*").c_str(), &findFileData);

    if (hFind == INVALID_HANDLE_VALUE) {
        return false;
    }

    do {
        if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            std::wstring filePath = directoryPath + L"\\" + findFileData.cFileName;
            DeleteFile(filePath.c_str());
        }
    } while (FindNextFile(hFind, &findFileData) != 0);

    FindClose(hFind);
    return true;
}

bool formatDisk(char driveLetter) {
    std::lock_guard<std::mutex> lock(mtx); // Block other threads

    std::wstring drivePath = L"";
    drivePath += static_cast<wchar_t>(driveLetter);
    drivePath += L":\\";

    return deleteFilesInDirectory(drivePath);
}

void writeRandomDataToFile(char driveLetter) {
    string filePath = string(1, driveLetter) + ":\\TEST.bin";
    const size_t bufferSize = 16 * 1024 * 1024; // 16 MB buffer
    char* buffer = new char[bufferSize];
    default_random_engine generator;
    uniform_int_distribution<int> distribution(0, 255);

    for (size_t i = 0; i < bufferSize; i++) {
        buffer[i] = static_cast<char>(distribution(generator));
    }

    cout << "Writing random data to file " << filePath << " using file system:" << endl;

    startTime = chrono::high_resolution_clock::now();
    thread printThread(printStatus); // Start the printStatus function in a separate thread

    while (true) {
        ofstream outFile(filePath, ios::binary | ios::out | ios::app);
        if (!outFile.is_open()) {
            cout << endl <<"Unable to open file for writing. Attempting to delete all files on the disk..." << endl;
            if (!formatDisk(driveLetter)) {
                cerr << "Error: Write and deleting failed. Stopping..." << endl;
                break;
            }
            continue;
        }

        outFile.write(buffer, bufferSize);
        if (!outFile) {
            cout << endl << "Unable to open file for writing. Attempting to delete all files on the disk..." << endl;
            outFile.close();
            if (!formatDisk(driveLetter)) {
                cerr << "Error: Write and deleting failed. Stopping..." << endl;
                break;
            }
            continue;
        }
        outFile.close();

        totalWrites += bufferSize;
    }

    continuePrinting = false;
    printThread.join();

    delete[] buffer;
}

int main() {
    ios::sync_with_stdio(false); // Disable sync with C streams for performance
    listDisks();
    char driveLetter;
    cin >> driveLetter;
    driveLetter = toupper(driveLetter);
    writeRandomDataToFile(driveLetter);
    return 0;
}