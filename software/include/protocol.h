#pragma once
#include <cstdint>

// Atributo de GCC para evitar el relleno (padding) automático del sistema operativo.
// Asegura que los datos en memoria contigua midan exactamente 10 bytes.
struct __attribute__((__packed__)) FpgaTickPacket {
    uint8_t sync_magic;  // 0x54 ('T') - Byte de sincronismo
    uint8_t order_type;  // 'B' (Bid) o 'A' (Ask)
    double  price;       // 8 bytes (IEEE 754 64-bit Little Endian nativo x86_64)
};

// Barrera de seguridad en tiempo de compilación. 
// Si alguien modifica el struct, fallará el build antes de corromper el UART.
static_assert(sizeof(FpgaTickPacket) == 10, "FpgaTickPacket MUST be exactly 10 bytes");