#include "ibkr_client.h"
#include <iostream>
#include <chrono>
#include <thread>

int main(int argc, char** argv) {
    std::string uartPort = (argc > 1) ? argv[1] : "/dev/ttyUSB1";
    std::cout << "Iniciando Artix7-Futures-Accelerator Host Engine..." << std::endl;

    UartInterface uart;
    // Intenta abrir el puerto serie; si la placa no está conectada, continúa en modo visualización
    if (!uart.openPort(uartPort)) {
        std::cout << "[!] Ejecutando en modo SIMULACIÓN (sin FPGA conectada)." << std::endl;
    }

    IbkrClient client(&uart);
    
    // Conexión a IB Gateway (Headless) en localhost, puerto 4002. Client ID 1.
    if (client.connect("127.0.0.1", 4002, 1)) {
        std::cout << "Conectado exitosamente al IB Gateway." << std::endl;

        // Esperar estabilización del socket
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        // Solicitar datos retrasados gratuitos (3) si no hay subscripcion live, o (1) para live
        client.client()->reqMarketDataType(3);

        // Definición del Contrato de Micro Futuro (MES)
        Contract contract;
        contract.symbol = "MES";
        contract.secType = "FUT";
        contract.exchange = "CME";
        contract.currency = "USD";
        contract.lastTradeDateOrContractMonth = "202609"; // Ajustar al vencimiento activo
        
        std::cout << "[+] Suscribiendo a flujo de mercado (MES)..." << std::endl;
        client.client()->reqMktData(1001, contract, "", false, false, TagValueListSPtr());

        // Mantener el hilo principal vivo (simulación)
        while(true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    } else {
        std::cerr << "[-] Error al conectar con IB Gateway en 127.0.0.1:4002." << std::endl;
        return 1;
    }

    return 0;
}