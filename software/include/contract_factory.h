#pragma once
#include <sstream>
#include <string>
#include <twsapi/Contract.h>

class ContractFactory {
  public:
    // Micro futuros CME (Seguidor en el Arbitraje: MES, MCL, etc..)
    static Contract makeMicroFuture(const std::string &symbol = "MES",
                                    const std::string &expiry = "202609",
                                    const std::string &exchange = "CME",
                                    const std::string &currency = "USD") {
        Contract c;
        c.symbol = symbol;
        c.secType = "FUT";
        c.exchange = exchange;
        c.currency = currency;
        c.lastTradeDateOrContractMonth = expiry;
        return c;
    }

    // Futuros institucionales CME/NYMEX (Lider en el Arbitraje: ES, CL, etc..)
    static Contract makeFuture(const std::string &symbol = "ES",
                               const std::string &expiry = "202609",
                               const std::string &exchange = "CME",
                               const std::string &currency = "USD") {
        Contract c;
        c.symbol = symbol;
        c.secType = "FUT";
        c.exchange = exchange;
        c.currency = currency;
        c.lastTradeDateOrContractMonth = expiry;
        return c;
    }

    // Criptomonedas 24/7 (PAXOS)
    static Contract makeCrypto(const std::string &symbol = "BTC",
                               const std::string &exchange = "PAXOS",
                               const std::string &currency = "USD") {
        Contract c;
        c.symbol = symbol;
        c.secType = "CRYPTO";
        c.exchange = exchange;
        c.currency = currency;
        return c;
    }

    // Forex Spot (IDEALPRO)
    static Contract makeForex(const std::string &base = "EUR", const std::string &quote = "USD",
                              const std::string &exchange = "IDEALPRO") {
        Contract c;
        c.symbol = base;
        c.secType = "CASH";
        c.exchange = exchange;
        c.currency = quote;
        return c;
    }

    // Acciones / ETFs (SMART Routing)
    static Contract makeStock(const std::string &symbol = "AAPL",
                              const std::string &exchange = "SMART",
                              const std::string &currency = "USD") {
        Contract c;
        c.symbol = symbol;
        c.secType = "STK";
        c.exchange = exchange;
        c.currency = currency;
        return c;
    }

    static std::string dumpContract(const Contract &c) {
        std::ostringstream oss;
        oss << "[" << c.secType << "] " << c.symbol;
        if (!c.lastTradeDateOrContractMonth.empty()) {
            oss << " (Exp: " << c.lastTradeDateOrContractMonth << ")";
        }
        oss << " @ " << c.exchange << " (" << c.currency << ")";
        return oss.str();
    }
};