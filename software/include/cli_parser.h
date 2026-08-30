#pragma once
#include <iostream>
#include <string>

struct CliConfig {
    std::string symbol;
    std::string expiry;
    std::string port = "/dev/ttyUSB1";
    bool valid = true;
    bool helpRequested = false;
};

class CliParser {
  public:
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
                cfg.port = argv[++i];
            } else {
                std::cerr << "[-] Parámetro no reconocido o argumento faltante: " << arg
                          << std::endl;
                cfg.valid = false;
                return cfg;
            }
        }

        return cfg;
    }

    static void printUsage(const char *progName) {
        std::cout << "\nUso: " << progName << " -s <SIMBOLO> -e <YYYYMM> [-p <PUERTO>]\n\n"
                  << "Opciones obligatorias:\n"
                  << "  -s, --symbol   Ticker del instrumento (ej: MES, ES, CL, BTC)\n"
                  << "  -e, --expiry   Mes de vencimiento en formato YYYYMM (ej: 202609)\n\n"
                  << "Opciones opcionales:\n"
                  << "  -p, --port     Dispositivo UART (por defecto: /dev/ttyUSB1)\n"
                  << "  -h, --help     Muestra este menú de ayuda\n"
                  << std::endl;
    }
};