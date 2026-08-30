#pragma once
#include <iostream>
#include <string>

// Estructura para almacenar la configuración del cliente
struct CliConfig {
    std::string symbol;         // Simbolo del instrumento
    std::string expiry;         // Fecha de vencimiento
    std::string uartPort;       // Puerto UART
    int ibPort;                 // Puerto IBKR
    int clientId;               // ID de cliente
    bool valid;                 // Validez de la configuración
    bool helpRequested = false; // Solicitud de ayuda
};

// Clase para parsear la configuración del cliente
class CliParser {
  public:
    // Método estático para parsear la configuración del cliente
    static CliConfig parse(int argc, char **argv) {
        CliConfig cfg;

        if (argc == 1) {
            cfg.valid = false;
            return cfg;
        }

        for (int i = 1; i < argc; i++) {
            std::string arg = argv[i];

            if (arg == "-h" || arg == "--help") {
                cfg.helpRequested = true;
                return cfg;
            } else if ((arg == "-s" || arg == "--symbol") && i + 1 < argc) {
                cfg.symbol = argv[++i];
            } else if ((arg == "-e" || arg == "--expiry") && i + 1 < argc) {
                cfg.expiry = argv[++i];
            } else if ((arg == "-p" || arg == "--port") && i + 1 < argc) {
                cfg.uartPort = argv[++i];
            } else if ((arg == "-i" || arg == "--ib-port") && i + 1 < argc) {
                try {
                    cfg.ibPort = std::stoi(argv[++i]);
                } catch (...) {
                    std::cerr << "[-] Error: El puerto de IBKR debe ser un valor numérico."
                              << std::endl;
                    cfg.valid = false;
                    return cfg;
                }
            } else {
                std::cerr << "[-] Parámetro no reconocido o argumento faltante: " << arg
                          << std::endl;
                cfg.valid = false;
                return cfg;
            }
        }

        return cfg;
    }

    // Método estático para imprimir el menú de ayuda
    static void printUsage(const char *progName) {
        std::cout << "\nUso: " << progName << " -s <SIMBOLO> -e <YYYYMM> [OPCIONES]\n\n"
                  << "Parámetros obligatorios:\n"
                  << "  -s, --symbol     Ticker del Micro Futuro (ej: MES, MCL, MYM)\n"
                  << "  -e, --expiry     Mes de vencimiento en formato YYYYMM (ej: 202609)\n\n"
                  << "Opciones configurables:\n"
                  << "  -i, --ib-port    Puerto del socket de IBKR (por defecto: 4002)\n"
                  << "                   - 4002: IB Gateway Paper (Simulación)\n"
                  << "                   - 4001: IB Gateway Live (Producción)\n"
                  << "                   - 7497: TWS Paper\n"
                  << "                   - 7496: TWS Live\n"
                  << "  -p, --port       Ruta del puerto UART/FTDI (por defecto: /dev/ttyUSB1)\n"
                  << "  -h, --help       Muestra este menú de ayuda\n"
                  << std::endl;
    }
};