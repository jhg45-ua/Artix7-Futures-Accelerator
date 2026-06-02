#include "ibkr_client.h"
#include <iostream>
#include <chrono>
#include <thread>

int main() {
    std::cout << "Iniciando Artix7-Futures-Accelerator Host Engine..." << std::endl;

    IbkrClient client;
    
    // Conexión a IB Gateway (Headless) en localhost, puerto 4002. Client ID 1.
    if (client.connect("127.0.0.1", 4002, 1)) {
        std::cout << "Conectado exitosamente al IB Gateway." << std::endl;
        
        // El hilo de lectura está corriendo asíncronamente en background.
        // Aquí puedes realizar las peticiones de suscripción al contrato MES.
        // client.m_client->reqMktData(...);
        
        // Mantener el hilo principal vivo (simulación)
        while(true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    } else {
        std::cerr << "Fallo al conectar con IB Gateway. ¿Está corriendo en puerto 4002?" << std::endl;
        return 1;
    }

    return 0;
}