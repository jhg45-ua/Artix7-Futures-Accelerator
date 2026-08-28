#pragma once
#include "protocol.h"
#include <string>

class UartInterface {
public:
    UartInterface();
    ~UartInterface();

    bool openPort(const std::string& portName, int baudRate = 115200);
    void closePort();
    bool sendPacket(const FpgaTickPacket& packet);
    bool isOpen() const { return m_fd != -1; }

private:
    int m_fd;   
};