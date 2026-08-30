#include "uart_interface.h"
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <string>
#include <termios.h>
#include <unistd.h>

UartInterface::UartInterface() : m_fd(-1) {};

UartInterface::~UartInterface() {
    closePort();
}

bool UartInterface::openPort(const std::string_view &portName, int baudRate) {
    (void)baudRate; // Fijado a 115200
    std::string portPath(portName);

    // O_RDWR: Lectura/Escritura bidireccional
    // O_NOCTTY: El puerto no toma control de la terminal
    // O_NDELAY: Apertura inmediata sin esperar DCD
    m_fd = open(portPath.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
    if (m_fd == -1) {
        std::cerr << "[-] Error abriendo UART en " << portName << ": " << strerror(errno) << "\n";
        return false;
    }

    // Modo bloqueante para escrituras atomicas
    fcntl(m_fd, F_SETFL, 0);

    struct termios tty;
    if (tcgetattr(m_fd, &tty) != 0) {
        std::cerr << "[-] Error tcgetattr: " << strerror(errno) << std::endl;
        closePort();
        return false;
    }

    // Configurar velocidad de 115200 baudios
    cfsetispeed(&tty, B115200);
    cfsetospeed(&tty, B115200);

    // Formato 8N1 sin control de flujo por hardwaret
    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~PARENB; // Sin paridad
    tty.c_cflag &= ~CSTOPB; // 1 bit de parada
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;      // 8 bits de datos
    tty.c_cflag &= ~CRTSCTS; // Sin RTS/CTS

    // Modo RAW Puro: Deshabilitar buffers de línea, ecos y señales
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    tty.c_iflag &= ~(IXON | IXOFF | IXANY | ICRNL | INPCK | ISTRIP);
    tty.c_oflag &= ~OPOST;

    // Timeouts
    tty.c_cc[VMIN] = 1;
    tty.c_cc[VTIME] = 0;

    if (tcsetattr(m_fd, TCSANOW, &tty) != 0) {
        std::cerr << "[-] Error tcsetattr: " << strerror(errno) << std::endl;
        closePort();
        return false;
    }

    std::cout << "[+] UART conectada en " << portName << " a 115200 baudios (Modo RAW)."
              << std::endl;
    return true;
}

void UartInterface::closePort() noexcept {
    if (m_fd != -1) {
        close(m_fd);
        m_fd = -1;
    }
}

bool UartInterface::sendPacket(const FpgaTickPacket &packet) const noexcept {
    if (m_fd == -1)
        return false;

    // Escritura atomica de 10 bytes directos al chip FTDI de la placa
    ssize_t bytesSent = write(m_fd, &packet, sizeof(FpgaTickPacket));
    return (bytesSent == static_cast<ssize_t>(sizeof(FpgaTickPacket)));
}