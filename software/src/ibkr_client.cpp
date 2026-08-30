#include "ibkr_client.h"
#include "protocol.h"
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iostream>
#include <sstream>

IbkrClient::IbkrClient(UartInterface *uart)
    : m_osSignal(2000), m_running(false), m_uart(uart), m_valDone(false),
      m_activeValidationReqId(-1), m_contractValid(false) {
    m_client = std::unique_ptr<EClientSocket>(new EClientSocket(this, &m_osSignal));
}

IbkrClient::~IbkrClient() {
    disconnect();
}

bool IbkrClient::connect(const char *host, int port, int clientId) {
    bool connected = m_client->eConnect(host, port, clientId, false);

    if (connected) {
        m_running = true;
        // Inicializamos el lector secundario
        m_reader = std::unique_ptr<EReader>(new EReader(m_client.get(), &m_osSignal));
        m_reader->start();

        // Hilo productor de red
        m_readerThread = std::thread(&IbkrClient::processMessages, this);

        // Hilo consumidor de UART
        m_uartThread = std::thread(&IbkrClient::uartWorker, this);
    }
    return connected;
}

void IbkrClient::disconnect() {
    if (m_running.exchange(false)) {
        if (m_client && m_client->isConnected()) {
            m_client->eDisconnect();
        }
        m_osSignal.issueSignal();

        if (m_readerThread.joinable()) {
            m_readerThread.join();
        }
        if (m_uartThread.joinable()) {
            m_uartThread.join();
        }
    }
}

void IbkrClient::processMessages() {
    while (m_client->isConnected()) {
        // Bloquea el hilo hasta que EReaderOSSignal despierta vía variable de condición
        m_osSignal.waitForSignal();
        m_reader->processMsgs(); // Decodifica y dispara callbacks como tickPrice
    }
}

void IbkrClient::uartWorker() {
    FpgaTickPacket packet;

    while (m_running.load(std::memory_order_relaxed)) {
        if (m_spscQueue.pop(packet)) {
            if (m_uart && m_uart->isOpen()) {
                m_uart->sendPacket(packet);
            }
        } else {
// Instrucción de pausa de CPU para mitigar consumo de pipeline en x86
#if defined(__x86_64__) || defined(_M_X64)
            __builtin_ia32_pause();
#else
            std::this_thread::yield();
#endif
        }
    }

    // Drenar los paquetes restantes en la cola antes de terminar
    while (m_spscQueue.pop(packet)) {
        if (m_uart && m_uart->isOpen()) {
            m_uart->sendPacket(packet);
        }
    }
}

bool IbkrClient::validateContract(int reqId, const Contract &contract, int timeoutMs) {
    {
        std::lock_guard<std::mutex> lock(m_valMutex);
        m_valDone = false;
        m_contractValid = false;
        m_activeValidationReqId = reqId;
    }

    m_client->reqContractDetails(reqId, contract);
    std::unique_lock<std::mutex> lock(m_valMutex);
    bool finished = m_valCv.wait_for(lock, std::chrono::milliseconds(timeoutMs),
                                     [this]() { return m_valDone; });

    if (!finished) {
        std::cerr << "[-] TIMEOUT: No hubo respuesta de IBKR al validar el contrato." << std::endl;
        m_contractValid = false;
        return false;
    }

    return m_contractValid.load();
}

// Control y Verificación de Horario de Mercado
std::string IbkrClient::getCurrentTimeInTz(const std::string &tzId) const {
    time_t now = time(nullptr);

    // Normalización de identificadores de timezone de IBKR para glibc/Linux
    std::string zoneName = tzId;
    if (zoneName == "US/Central" || zoneName == "CST") {
        zoneName = "America/Chicago";
    } else if (zoneName == "US/Eastern" || zoneName == "EST") {
        zoneName = "America/New_York";
    } else if (zoneName == "US/Pacific" || zoneName == "PST") {
        zoneName = "America/Los_Angeles";
    }

    // El prefijo ':' obliga a glibc a buscar el archivo exacto en /usr/share/zoneinfo/
    std::string tzRule = ":" + zoneName;

    char *prevTz = getenv("TZ");
    std::string oldTz = prevTz ? prevTz : "";

    setenv("TZ", tzRule.c_str(), 1);
    tzset();

    struct tm tmInfo;
    localtime_r(&now, &tmInfo);

    if (!oldTz.empty()) {
        setenv("TZ", oldTz.c_str(), 1);
    } else {
        unsetenv("TZ");
    }
    tzset();

    char buf[32];
    strftime(buf, sizeof(buf), "%Y%m%d:%H%M", &tmInfo);
    return std::string(buf);
}

bool IbkrClient::isSessionActive(const std::string &hoursStr, const std::string &tzId) const {
    if (hoursStr.empty())
        return true; // Activos 24/7 sin restricción (ej. Cripto)

    std::string nowStr = getCurrentTimeInTz(tzId);
    std::stringstream ss(hoursStr);
    std::string token;

    std::cout << "    [DEBUG] Hora en Exchange (" << tzId << "): " << nowStr << std::endl;

    while (std::getline(ss, token, ';')) {
        if (token.empty())
            continue;

        if (token.find("CLOSED") != std::string::npos) {
            continue;
        }

        size_t dash = token.find('-');
        if (dash != std::string::npos) {
            std::string startStr = token.substr(0, dash);
            std::string endStr = token.substr(dash + 1);

            if (nowStr >= startStr && nowStr < endStr) {
                std::cout << "    [DEBUG] Coincidencia con sesión: " << startStr << " -> " << endStr
                          << std::endl;
                return true;
            }
        }
    }
    return false;
}

bool IbkrClient::isMarketOpen(bool checkLiquidOnly) const {
    if (!m_contractValid.load())
        return false;

    const std::string &schedule =
        checkLiquidOnly ? m_activeDetails.liquidHours : m_activeDetails.tradingHours;
    return isSessionActive(schedule, m_activeDetails.timeZoneId);
}

void IbkrClient::tickPrice(int tickerId, TickType field, double price, const TickAttrib &attrib) {
    // Aquí recibimos el Tick de Micro Futuros.
    // field == 1 (Bid), field == 2 (Ask).
    // Indicamos explícitamente al compilador que ignoramos estos parámetros de la interfaz por
    // ahora
    (void)tickerId;
    (void)attrib;

    if (!m_contractValid.load(std::memory_order_relaxed) || price <= 0.0) {
        return;
    }

    if (field == 1 || field == 2 || field == 66 || field == 67) {
        FpgaTickPacket packet;
        packet.sync_magic = 0x54;
        packet.order_type = (field == 1 || field == 66) ? 'B' : 'A';
        packet.price = price;

        // Encolar de forma no bloqueante hacia el hilo UART
        m_spscQueue.push(packet);

        // Salida ligera para monitorización
        std::cout << "[TICK -> FPGA] Tipo: " << packet.order_type << " | Precio: " << packet.price
                  << std::endl;
    }
}

void IbkrClient::contractDetails(int reqId, const ContractDetails &details) {
    std::lock_guard<std::mutex> lock(m_valMutex);
    if (reqId == m_activeValidationReqId) {
        m_activeDetails = details;
        m_contractValid = true;

        std::cout << "\n[+] === CONTRATO VALIDADO EN IBKR ===" << std::endl;
        std::cout << "    - ConId:         " << details.contract.conId << std::endl;
        std::cout << "    - Ticker Local:  " << details.contract.localSymbol << std::endl;
        std::cout << "    - Zona Horaria:  " << details.timeZoneId << std::endl;
        std::cout << "    - Trading Hours: " << details.tradingHours << std::endl;
        std::cout << "    - Liquid Hours:  " << details.liquidHours << std::endl;
        std::cout << "    - Min Tick Size: " << details.minTick << "\n" << std::endl;
    }
}

void IbkrClient::contractDetailsEnd(int reqId) {
    std::lock_guard<std::mutex> lock(m_valMutex);
    if (reqId == m_activeValidationReqId) {
        m_valDone = true;
        m_valCv.notify_all();
    }
}

void IbkrClient::error(int id, long long errorTimeMs, int errorCode, const std::string &errorString,
                       const std::string &advancedOrderRejectJson) {
    (void)errorTimeMs;
    (void)advancedOrderRejectJson;

    if (id == m_activeValidationReqId && errorCode == 200) {
        std::lock_guard<std::mutex> lock(m_valMutex);
        m_contractValid = false;
        m_valDone = true;
        m_valCv.notify_all();
    }

    switch (errorCode) {
    case 200:
        std::cout << "[-] ERROR [200] (reqId=" << id
                  << "): Contrato no reconocido o parámetros inválidos (" << errorString << ")"
                  << std::endl;
        break;
    case 354:
        std::cout << "[-] AVISO [354]: Sin suscripción de tiempo real activa. Cambie a Delayed "
                     "(reqMarketDataType 3)."
                  << std::endl;
        break;
    case 10167:
        std::cout
            << "[i] INFO [10167]: Datos de mercado en diferido (Delayed) confirmados por IBKR."
            << std::endl;
        break;
    case 2104:
    case 2106:
    case 2107:
    case 2108:
    case 2119:
    case 2158:
        break;
    default:
        if (errorCode >= 2000) {
            std::cout << "[i] Notificación IBKR [" << errorCode << "]: " << errorString
                      << std::endl;
        } else {
            std::cout << "[-] Error API [" << errorCode << "]: " << errorString << std::endl;
        }
        break;
    }
}