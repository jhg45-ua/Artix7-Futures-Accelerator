#pragma once
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

class IbkrClient : public DefaultEWrapper {
  public:
    explicit IbkrClient(UartInterface *uart = nullptr);
    ~IbkrClient();

    bool connect(const char *host, int port, int clientId);
    void disconnect();
    void setUart(UartInterface *uart) { m_uart = uart; }
    EClientSocket *client() { return m_client.get(); }

    bool validateContract(int reqId, const Contract &contract, int timeoutMs = 3000);
    bool isMarketOpen(bool checkLiquidOnly = false) const;
    bool isContractValid() const { return m_contractValid.load(); };
    const ContractDetails &getContractDetails() const { return m_activeDetails; }

    using DefaultEWrapper::error;

    // Callbacks de EWrapper sobrecargados
    void tickPrice(int tickerId, TickType field, double price, const TickAttrib &attrib) override;

    // Callbacks de Contracts sobrecargados
    void contractDetails(int reqId, const ContractDetails &contractDetails) override;
    void contractDetailsEnd(int reqId) override;

    // Callback de gestion de errores
    void error(int id, long long errorTimeMs, int errorCode, const std::string &errorString,
               const std::string &advancedOrderRejectJson) override;

  private:
    void processMessages();
    bool isSessionActive(const std::string &hoursStr, const std::string &tzId) const;
    std::string getCurrentTimeTz(const std::string &tzId) const;

    EReaderOSSignal m_osSignal;
    std::unique_ptr<EClientSocket> m_client;
    std::unique_ptr<EReader> m_reader;
    std::thread m_readerThread;
    std::atomic<bool> m_running;

    UartInterface *m_uart;

    std::mutex m_valMutex;
    std::condition_variable m_valCv;
    bool m_valDone;
    int m_activeValidationReqId;
    std::atomic<bool> m_contractValid;
    ContractDetails m_activeDetails;
};