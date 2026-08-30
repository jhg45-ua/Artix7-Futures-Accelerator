#pragma once
#include "lockfree_queue.h"
#include "order_book.h"
#include "uart_interface.h"
#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
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
    bool connect(const char *host, int port, int clientId);
    // Método para desconectar del IBKR
    void disconnect();
    // Método para establecer la UART
    void setUart(UartInterface *uart) { m_uart = uart; }
    // Método para obtener el cliente de IBKR
    EClientSocket *client() { return m_client.get(); }

    // Método para validar el contrato
    bool validateContract(int reqId, const Contract &contract, int timeoutMs = 3000);
    // Método para verificar si el mercado está abierto
    bool isMarketOpen(bool checkLiquidOnly = false) const;
    // Método para verificar si el contrato es válido
    bool isContractValid() const { return m_contractValid.load(); };
    // Método para obtener los detalles del contrato
    const ContractDetails &getContractDetails() const { return m_activeDetails; }

    // Acceso al estado actual del libro de órdenes
    const OrderBook &getOrderBook() const { return m_orderBook; }

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
    bool isSessionActive(const std::string &hoursStr, const std::string &tzId) const;
    // Método para obtener la hora actual en una zona horaria
    std::string getCurrentTimeInTz(const std::string &tzId) const;

    // Señal para notificar al hilo de lectura
    EReaderOSSignal m_osSignal;
    // Cliente de IBKR
    std::unique_ptr<EClientSocket> m_client;
    // Lector de mensajes de IBKR
    std::unique_ptr<EReader> m_reader;
    // Hilo para procesar mensajes de IBKR
    std::thread m_readerThread;
    // Hilo consumidor
    std::thread m_uartThread;
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