#pragma once
#include "protocol.h"
#include <string>

// Clase que define la interfaz de comunicacion serial entre el PC y la FPGA por medio de UART
class UartInterface {
  public:
    // Constructor
    UartInterface();
    // Destructor
    ~UartInterface();

    // Método para abrir el puerto serial
    bool openPort(const std::string &portName, int baudRate = 115200);
    // Método para cerrar el puerto serial
    void closePort();
    // Método para enviar un paquete a la FPGA
    bool sendPacket(const FpgaTickPacket &packet);
    // Método para verificar si el puerto serial está abierto
    bool isOpen() const { return m_fd != -1; }

  private:
    // Descriptor de archivo del puerto serial
    int m_fd;
};