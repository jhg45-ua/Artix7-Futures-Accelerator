#pragma once
#include "protocol.h"
#include <cstdint>

class OrderBook {
  public:
    constexpr OrderBook() noexcept
        : m_bestBid(0.0), m_bestAsk(0.0), m_hasBid(false), m_hasAsk(false) {}

    // Procesa un nuevo tick. Retorna true solo si el libro queda balanceado (Ask > Bid)
    // y rellena el struct FpgaTickPacket listo para encolar.
    [[nodiscard]] bool processTick(char side, double price, FpgaTickPacket &outPacket) noexcept {
        // 1. Descartar precios centinela (-1.0) o cotizaciones inválidas
        if (price <= 0.0) [[unlikely]]
            return false;

        // Actualizar el estado interno del libro
        if (side == 'B') [[likely]] {
            m_bestBid = price;
            m_hasBid = true;
        } else if (side == 'A') [[likely]] {
            m_bestAsk = price;
            m_hasAsk = true;
        } else [[unlikely]]
            return false;

        // Validar consistencia de mercado: ambos lados presentes y Ask > Bid
        if (!isBalanced()) [[unlikely]]
            // Si el libro está cruzado (Bid >= Ask) por interleaving, descartamos el tick
            return false;

        // Construir el paquete binario de 10 bytes para la FPGA
        outPacket.sync_magic = 0x54; // 'T'
        outPacket.order_type = static_cast<uint8_t>(side);
        outPacket.price = price;

        return true;
    }

    [[nodiscard]] constexpr double getBid() const noexcept { return m_bestBid; }
    [[nodiscard]] constexpr double getAsk() const noexcept { return m_bestAsk; }

    [[nodiscard]] constexpr double getSpread() const noexcept {
        return (m_hasBid && m_hasAsk) ? (m_bestAsk - m_bestBid) : 0.0;
    }

    [[nodiscard]] constexpr bool isBalanced() const noexcept {
        return m_hasBid && m_hasAsk && (m_bestAsk > m_bestBid);
    }

    constexpr void reset() noexcept {
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