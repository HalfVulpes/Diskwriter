# Disk Write Test

## Description

This program writes random data to a specified disk until manually stopped. It is designed to test the write speed and reliability of storage devices.

## Features

- Lists available local drives with their type and capacity.
- Writes random data to a specified drive in 16 MB chunks.
- Displays real-time statistics, including total writes, current speed, average speed, and time elapsed.
- If writing fails, delete existing files on the disk, then attempt to write again.

## Usage

1. Compile the program using a C++ compiler that supports C++11 or higher.
2. Run the executable. The program will list available drives.
3. Enter the drive's letter you want to test (e.g., `E` for `E:` drive).
4. The program will start writing random data to the specified drive. Press `Ctrl+C` to stop the program.

## Requirements

- Windows operating system
- A C++ compiler that supports C++11 or higher

## Disclaimer

This program writes data continuously to the specified disk and may delete existing files if writing fails. Use it at your own risk and ensure you have data backed up.
