#pragma once
#include <cstdint>

// Cabecera que define la estructura de datos y protocolos de comunicación entre el PC y la FPGA

// Definición de la estructura de datos que se envia a la FPGA
struct __attribute__((__packed__)) FpgaTickPacket {
    // Atributo de GCC para evitar el relleno (padding) automático del sistema operativo.
    // Asegura que los datos en memoria contigua midan exactamente 10 bytes.
    uint8_t sync_magic; // 0x54 ('T') - Byte de sincronismo
    uint8_t order_type; // 'B' (Bid) o 'A' (Ask)
    double price;       // 8 bytes (IEEE 754 64-bit Little Endian nativo x86_64)
};

// Barrera de seguridad en tiempo de compilación.
// Si alguien modifica el struct, fallará el build antes de corromper el UART.
static_assert(sizeof(FpgaTickPacket) == 10, "FpgaTickPacket MUST be exactly 10 bytes");