#include "ibkr_client.h"
#include "protocol.h"
#include <iostream>

IbkrClient::IbkrClient() : m_osSignal(2000) { // Timeout de 2000ms para la señal
    m_client = std::unique_ptr<EClientSocket>(new EClientSocket(this, &m_osSignal));
}

IbkrClient::~IbkrClient() {
    if (m_client->isConnected()) {
        m_client->eDisconnect();
    }
}

bool IbkrClient::connect(const char* host, int port, int clientId) {
    bool connected = m_client->eConnect(host, port, clientId, false);
    
    if (connected) {
        // Inicializamos el lector secundario
        m_reader = std::unique_ptr<EReader>(new EReader(m_client.get(), &m_osSignal));
        m_reader->start();

        // Lanzamos el hilo secundario que esperará en el socket crudo de IBKR
        m_readerThread = std::thread(&IbkrClient::processMessages, this);
        m_readerThread.detach();
    }
    return connected;
}

void IbkrClient::processMessages() {
    while (m_client->isConnected()) {
        // Bloquea el hilo hasta que EReaderOSSignal despierta vía variable de condición
        m_osSignal.waitForSignal(); 
        m_reader->processMsgs(); // Decodifica y dispara callbacks como tickPrice
    }
}

void IbkrClient::tickPrice(int tickerId, TickType field, double price, const TickAttrib& attrib) {
    // Aquí recibimos el Tick de Micro Futuros.
    // field == 1 (Bid), field == 2 (Ask).
    // Indicamos explícitamente al compilador que ignoramos estos parámetros de la interfaz por ahora
    (void)tickerId;
    (void)attrib;
    
    if (field == 1 || field == 2) {
        FpgaTickPacket packet;
        packet.sync_magic = 0x54;
        packet.order_type = (field == 1) ? 'B' : 'A';
        packet.price = price;

        // Forzamos el uso de la variable (temporalmente) para silenciar el warning
        // hasta que tengamos el descriptor del UART listo para el write()
        (void)packet;

        // TODO: Hacer write() del packet crudo hacia /dev/ttyUSBX
        // std::cout << "Enviando a FPGA -> Tipo: " << packet.order_type << " Precio: " << price << "\n";
    }
}