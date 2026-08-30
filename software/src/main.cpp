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
    if (client.connect("127.0.0.1", 7497, 1)) {
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

        // 1. Instanciar contrato con ContractFactory (Ej: MES o BTC)
        Contract target = ContractFactory::makeMicroFuture("MES", "202609");
        std::cout << "[*] Verificando especificaciones de: "
                  << ContractFactory::dumpContract(target) << std::endl;

        // 2. Validación síncrona determinista del contrato
        if (!client.validateContract(2001, target, 3000)) {
            std::cerr << "[-] VALIDACIÓN FALLIDA: Contrato rechazado por el broker." << std::endl;
            std::cerr << "[-] Cancelando pipeline." << std::endl;
            client.disconnect();
            return 1;
        }

        // 3. Verificación de Horario de Negociación
        bool tradingOpen = client.isMarketOpen(false); // Sesión completa (Globex / ETH)
        bool liquidOpen = client.isMarketOpen(true);   // Sesión regular de alta liquidez (RTH)

        std::cout << "[*] Estado de Sesión en Exchange (" << client.getContractDetails().timeZoneId
                  << "):" << std::endl;
        std::cout << "    - Negociación Electrónica (Trading Hours): "
                  << (tradingOpen ? "ABIERTO [OK]" : "CERRADO [!]") << std::endl;
        std::cout << "    - Sesión Líquida Regular (Liquid Hours):   "
                  << (liquidOpen ? "ABIERTO [OK]" : "CERRADO [!]") << std::endl;

        // Bloqueo y salida si la sesión de negociación está inactiva
        if (!tradingOpen) {
            std::cerr
                << "\n[-] MERCADO CERRADO: La sesión de trading para este activo no está activa."
                << std::endl;
            std::cerr << "[-] No se enviarán datos ni órdenes a la FPGA. Cancelando ejecución."
                      << std::endl;
            client.disconnect();
            return 1;
        }

        // 4. Suscribir a datos de mercado
        client.client()->reqMarketDataType(3); // 3 = Delayed, 1 = Real-Time
        client.client()->reqMktData(1001, target, "", false, false, TagValueListSPtr());

        std::cout << "[+] Pipeline operativo. Escuchando cotizaciones...\n" << std::endl;

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