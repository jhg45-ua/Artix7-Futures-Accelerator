#pragma once
#include <atomic>
#include <cstddef>

template <typename T, size_t Capacity = 1024> class LockFreeSPSCQueue {
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be a power of 2");
    static_assert(Capacity >= 2, "Capacity must be at least 2");

  public:
    LockFreeSPSCQueue() : m_head(0), m_tail(0) {}
    ~LockFreeSPSCQueue() = default;

    // Deshabilitar copia y asignación para garantizar seguridad concurrente
    LockFreeSPSCQueue(const LockFreeSPSCQueue &) = delete;
    LockFreeSPSCQueue &operator=(const LockFreeSPSCQueue &) = delete;

    // Productor: Ejecutando exclusivamente por el hilo EReader (tickPrice)
    bool push(const T &item) {
        const size_t currentTail = m_tail.load(std::memory_order_relaxed);
        const size_t currentHead = m_head.load(std::memory_order_acquire);

        // Comprobar si el buffer esta lleno
        if ((currentTail - currentHead) == Capacity) {
            return false; // Buffer overflow (descarte preventivo)
        }

        m_buffer[currentTail & BufferMask] = item;
        m_tail.store(currentTail + 1, std::memory_order_release);
        return true;
    }

    // Consumidor: Ejecutado exclusivamente por el hilo dedicado de la UART
    bool pop(T &item) {
        const size_t currentHead = m_head.load(std::memory_order_relaxed);
        const size_t currentTail = m_tail.load(std::memory_order_acquire);

        // Comprobar si el buffer está vacío
        if (currentHead == currentTail) {
            return false;
        }

        item = m_buffer[currentHead & BufferMask];
        m_head.store(currentHead + 1, std::memory_order_release);
        return true;
    }

    bool empty() const {
        return m_head.load(std::memory_order_relaxed) == m_tail.load(std::memory_order_relaxed);
    }

    size_t size() const {
        size_t head = m_head.load(std::memory_order_relaxed);
        size_t tail = m_tail.load(std::memory_order_relaxed);
        return (tail >= head) ? (tail - head) : 0;
    }

  private:
    static constexpr size_t BufferMask = Capacity - 1;
    T m_buffer[Capacity];

    // alignas(64) aísla variables en líneas de caché L1 distintas (elimina False Sharing)
    alignas(64) std::atomic<size_t> m_head;
    alignas(64) std::atomic<size_t> m_tail;
};