#pragma once
#include "protocol.h"
#include <cstdint>

class OrderBook {
  public:
    OrderBook() : m_bestBid(0.0), m_bestAsk(0.0), m_hasBid(false), m_hasAsk(false) {}

    // Procesa un nuevo tick. Retorna true solo si el libro queda balanceado (Ask > Bid)
    // y rellena el struct FpgaTickPacket listo para encolar.
    bool processTick(char side, double price, FpgaTickPacket &outPacket) {
        // 1. Descartar precios centinela (-1.0) o cotizaciones inválidas
        if (price <= 0.0)
            return false;

        // Actualizar el estado interno del libro
        if (side == 'B') {
            m_bestBid = price;
            m_hasBid = true;
        } else if (side == 'A') {
            m_bestAsk = price;
            m_hasAsk = true;
        } else
            return false;

        // Validar consistencia de mercado: ambos lados presentes y Ask > Bid
        if (!isBalanced())
            // Si el libro está cruzado (Bid >= Ask) por interleaving, descartamos el tick
            return false;

        // Construir el paquete binario de 10 bytes para la FPGA
        outPacket.sync_magic = 0x54; // 'T'
        outPacket.order_type = static_cast<uint8_t>(side);
        outPacket.price = price;

        return true;
    }

    double getBid() const { return m_bestBid; }
    double getAsk() const { return m_bestAsk; }

    double getSpread() const { return (m_hasBid && m_hasAsk) ? (m_bestAsk - m_bestBid) : 0.0; }

    bool isBalanced() const { return m_hasBid && m_hasAsk && (m_bestAsk > m_bestBid); }

    void reset() {
        m_bestBid = 0.0;
        m_bestAsk = 0.0;
        m_hasBid = false;
        m_hasAsk = false;
    }

  private:
    double m_bestBid;
    double m_bestAsk;
    bool m_hasBid;
    bool m_hasAsk;
};