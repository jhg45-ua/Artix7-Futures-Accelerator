#pragma once
#include "uart_interface.h"
#include <memory>
#include <string>
#include <thread>
#include <twsapi/Contract.h>
#include <twsapi/DefaultEWrapper.h>
#include <twsapi/EClientSocket.h>
#include <twsapi/EReader.h>
#include <twsapi/EReaderOSSignal.h>

class IbkrClient : public DefaultEWrapper {
  public:
    explicit IbkrClient(UartInterface *uart = nullptr);
    ~IbkrClient();

    bool connect(const char *host, int port, int clientId);
    void disconnect();
    void setUart(UartInterface *uart) { m_uart = uart; }
    EClientSocket *client() { return m_client.get(); }

    using DefaultEWrapper::error;

    // Callbacks de EWrapper sobrecargados
    void tickPrice(int tickerId, TickType field, double price, const TickAttrib &attrib) override;

    // Callbacks de Contracts sobrecargados
    void contractDetails(int reqId, const ContractDetails &contractDetails) override;
    void contractDetailsEnd(int reqId) override;

    // Callback de gestion de errores
    void error(int id, int errorCode, const std::string &errorString,
               const std::string &advancedOrderRejectJson) override;
    void error(int id, int errorCode, const std::string &errorString) override;
    void error(const std::string &errorString) override;

  private:
    void processMessages();

    EReaderOSSignal m_osSignal;
    std::unique_ptr<EClientSocket> m_client;
    std::unique_ptr<EReader> m_reader;
    std::thread m_readerThread;

    UartInterface *m_uart;
};