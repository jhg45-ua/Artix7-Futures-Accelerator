#pragma once
#include "protocol.h"
#include <string_view>

// Clase que define la interfaz de comunicacion serial entre el PC y la FPGA por medio de UART
class UartInterface {
  public:
    // Constructor
    UartInterface();
    // Destructor
    ~UartInterface();

    // Método para abrir el puerto serial
    [[nodiscard]] bool openPort(const std::string_view &portName, int baudRate = 115200);
    // Método para cerrar el puerto serial
    void closePort() noexcept;
    // Método para enviar un paquete a la FPGA
    [[nodiscard]] bool sendPacket(const FpgaTickPacket &packet) const noexcept;
    // Método para verificar si el puerto serial está abierto
    [[nodiscard]] bool isOpen() const noexcept { return m_fd != -1; }

  private:
    // Descriptor de archivo del puerto serial
    int m_fd;
};