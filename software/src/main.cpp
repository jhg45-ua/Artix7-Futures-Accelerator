#include "contract_factory.h"
#include "ibkr_client.h"
#include <chrono>
#include <iostream>
#include <thread>

int main(int argc, char **argv) {
    std::string uartPort = (argc > 1) ? argv[1] : "/dev/ttyUSB1";
    std::cout << "Iniciando Artix7-Futures-Accelerator Host Engine..." << std::endl;

    UartInterface uart;
    // Intenta abrir el puerto serie; si la placa no está conectada, continúa en
    // modo visualización
    if (!uart.openPort(uartPort)) {
        std::cout << "[!] Ejecutando en modo SIMULACIÓN (sin FPGA conectada)." << std::endl;
    }

    IbkrClient client(&uart);

    // Conexión a IB Gateway (Headless) en localhost, puerto 4002. Client ID 1.
    if (client.connect("127.0.0.1", 4002, 1)) {
        std::cout << "Conectado exitosamente al IB Gateway." << std::endl;
        // Esperar estabilización del socket
        std::this_thread::sleep_for(std::chrono::milliseconds(300));

        // // Solicitar datos retrasados gratuitos (3) si no hay subscripcion live, o
        // // (1) para live
        // client.client()->reqMarketDataType(3);

        // // Generar contratos mediante la clase ContractFactory
        // Contract contractLeader = ContractFactory::makeCrypto("BTC");
        // Contract contractFollower = ContractFactory::makeCrypto("ETH");

        // std::cout << "[+] Contrato Lider: " << ContractFactory::dumbContract(contractLeader)
        //           << std::endl;
        // std::cout << "[+] Contrato Seguidor: " << ContractFactory::dumbContract(contractFollower)
        //           << std::endl;

        // // Suscribir a datos de mercado con IDs unívocos (ej. 1001 y 1002)
        // std::cout << "[+] Suscribiendo a flujos de mercado..." << std::endl;
        // client.client()->reqMktData(1001, contractLeader, "", false, false, TagValueListSPtr());
        // client.client()->reqMktData(1002, contractFollower, "", false, false,
        // TagValueListSPtr());

        // 1. Generar contrato con ContractFactory
        Contract target = ContractFactory::makeMicroFuture("MES", "202609");
        std::cout << "[*] Consultando especificaciones de: "
                  << ContractFactory::dumpContract(target) << std::endl;

        // 2. Validación síncrona determinista
        if (!client.validateContract(2001, target, 3000)) {
            std::cerr
                << "[-] VALIDACIÓN FALLIDA: El contrato no existe o el broker lo ha rechazado."
                << std::endl;
            std::cerr << "[-] Pipeline cancelado. No se solicitarán datos de mercado ni se "
                         "emitirán paquetes a la FPGA."
                      << std::endl;
            return 1;
        }

        // 3. Suscribir a flujo de datos (3 = Delayed gratuito)
        client.client()->reqMarketDataType(3);
        client.client()->reqMktData(1001, target, "", false, false, TagValueListSPtr());

        std::cout << "[+] Pipeline activo y blindado. Esperando cotizaciones...\n" << std::endl;

        // Mantener el hilo principal vivo (simulación)
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    } else {
        std::cerr << "[-] Error al conectar con IB Gateway en 127.0.0.1:4002." << std::endl;
        return 1;
    }

    return 0;
}