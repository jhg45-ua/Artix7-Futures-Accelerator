#pragma once
#include <twsapi/DefaultEWrapper.h>
#include <twsapi/EReaderOSSignal.h>
#include <twsapi/EReader.h>
#include <twsapi/EClientSocket.h>
#include <twsapi/Contract.h>
#include "uart_interface.h"
#include <memory>
#include <thread>

class IbkrClient : public DefaultEWrapper {
public:
    explicit IbkrClient(UartInterface* uart = nullptr);
    ~IbkrClient();

    bool connect(const char* host, int port, int clientId);
    void disconnect();
    void setUart(UartInterface* uart) { m_uart = uart; }
    EClientSocket* client() { return m_client.get(); }

    // Callbacks de EWrapper sobrecargados
    void tickPrice(int tickerId, TickType field, double price, const TickAttrib& attrib) override;

private:
    void processMessages();

    EReaderOSSignal m_osSignal;
    std::unique_ptr<EClientSocket> m_client;
    std::unique_ptr<EReader> m_reader;
    std::thread m_readerThread;

    UartInterface* m_uart;
};