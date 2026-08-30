#pragma once
#include "lockfree_queue.h"
#include "order_book.h"
#include "uart_interface.h"
#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>
#include <twsapi/Contract.h>
#include <twsapi/DefaultEWrapper.h>
#include <twsapi/EClientSocket.h>
#include <twsapi/EReader.h>
#include <twsapi/EReaderOSSignal.h>

// Clase que hereda de DefaultEWrapper para implementar los callbacks de IBKR
class IbkrClient : public DefaultEWrapper {
  public:
    // Constructor
    explicit IbkrClient(UartInterface *uart = nullptr);
    // Destructor
    ~IbkrClient();

    // Método para conectar al IBKR
    [[nodiscard]] bool connect(std::string_view host, int port, int clientId);
    // Método para desconectar del IBKR
    void disconnect();
    // Método para establecer la UART
    void setUart(UartInterface *uart) noexcept { m_uart = uart; }
    // Método para obtener el cliente de IBKR
    [[nodiscard]] EClientSocket *client() noexcept { return m_client.get(); }

    // Método para validar el contrato
    [[nodiscard]] bool validateContract(int reqId, const Contract &contract, int timeoutMs = 3000);
    // Método para verificar si el mercado está abierto
    [[nodiscard]] bool isMarketOpen(bool checkLiquidOnly = false) const;
    // Método para verificar si el contrato es válido
    [[nodiscard]] bool isContractValid() const noexcept { return m_contractValid.load(); };
    // Método para obtener los detalles del contrato
    [[nodiscard]] const ContractDetails &getContractDetails() const noexcept {
        return m_activeDetails;
    }

    // Acceso al estado actual del libro de órdenes
    [[nodiscard]] const OrderBook &getOrderBook() const noexcept { return m_orderBook; }

    // Callbacks de EWrapper sobrecargados
    using DefaultEWrapper::error;

    // Callback que se ejecuta cuando se reciben los detalles de un tick
    void tickPrice(int tickerId, TickType field, double price, const TickAttrib &attrib) override;

    // Callbacks de Contracts sobrecargados

    // Callback que se ejecuta cuando se reciben los detalles de un contrato
    void contractDetails(int reqId, const ContractDetails &contractDetails) override;
    // Callback que se ejecuta al finalizar la obtencion de detalles del contrato
    void contractDetailsEnd(int reqId) override;

    // Callback de gestion de errores
    void error(int id, long long errorTimeMs, int errorCode, const std::string &errorString,
               const std::string &advancedOrderRejectJson) override;

  private:
    // Método para procesar mensajes de IBKR
    void processMessages();
    // Bucle del consumidor dedicado a la UART
    void uartWorker();
    // Método para verificar si el mercado está activo
    [[nodiscard]] bool isSessionActive(std::string_view hoursStr, std::string_view tzId) const;
    // Método para obtener la hora actual en una zona horaria
    [[nodiscard]] std::string getCurrentTimeInTz(std::string_view tzId) const;

    // Señal para notificar al hilo de lectura
    EReaderOSSignal m_osSignal;
    // Cliente de IBKR
    std::unique_ptr<EClientSocket> m_client;
    // Lector de mensajes de IBKR
    std::unique_ptr<EReader> m_reader;
    // Hilo para procesar mensajes de IBKR
    std::jthread m_readerThread;
    // Hilo consumidor
    std::jthread m_uartThread;
    // Variable atómica para controlar el estado del hilo de lectura
    std::atomic<bool> m_running;

    // Interfaz UART para comunicar con la FPGA
    UartInterface *m_uart;
    LockFreeSPSCQueue<FpgaTickPacket, 1024> m_spscQueue; // Cola Lock-Free SPSC
    OrderBook m_orderBook;                               // Filtro de spreads y libro en memoria

    // Mutex y variable de condición para sincronización de validación de contratos
    std::mutex m_valMutex;
    std::condition_variable m_valCv;
    // Bandera para indicar que la validación del contrato ha finalizado
    bool m_valDone;
    // ID de la solicitud de validación del contrato
    int m_activeValidationReqId;
    // Bandera para indicar si el contrato es válido
    std::atomic<bool> m_contractValid;
    // Detalles del contrato activo
    ContractDetails m_activeDetails;
};