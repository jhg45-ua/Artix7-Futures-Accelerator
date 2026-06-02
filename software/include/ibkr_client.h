#pragma once
#include <twsapi/DefaultEWrapper.h>
#include <twsapi/EReaderOSSignal.h>
#include <twsapi/EReader.h>
#include <twsapi/EClientSocket.h>
#include <memory>
#include <thread>

class IbkrClient : public DefaultEWrapper {
public:
    IbkrClient();
    ~IbkrClient();

    bool connect(const char* host, int port, int clientId);
    void run();

    // Callbacks de EWrapper sobrecargados
    void tickPrice(int tickerId, TickType field, double price, const TickAttrib& attrib) override;

private:
    void processMessages();

    EReaderOSSignal m_osSignal;
    std::unique_ptr<EClientSocket> m_client;
    std::unique_ptr<EReader> m_reader;
    std::thread m_readerThread;
};