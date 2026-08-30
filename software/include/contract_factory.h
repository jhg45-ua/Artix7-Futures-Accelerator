#pragma once
#include <format>
#include <string>
#include <string_view>
#include <twsapi/Contract.h>

class ContractFactory {
  public:
    // Micro futuros CME (Seguidor en el Arbitraje: MES, MCL, etc..)
    [[nodiscard]] static Contract makeMicroFuture(const std::string_view &symbol = "MES",
                                                  const std::string_view &expiry = "202609",
                                                  const std::string_view &exchange = "CME",
                                                  const std::string_view &currency = "USD") {
        Contract c;
        c.symbol = symbol;
        c.secType = "FUT";
        c.exchange = exchange;
        c.currency = currency;
        c.lastTradeDateOrContractMonth = expiry;
        return c;
    }

    // Futuros institucionales CME/NYMEX (Lider en el Arbitraje: ES, CL, etc..)
    [[nodiscard]] static Contract makeFuture(const std::string_view &symbol = "ES",
                                             const std::string_view &expiry = "202609",
                                             const std::string_view &exchange = "CME",
                                             const std::string_view &currency = "USD") {
        Contract c;
        c.symbol = symbol;
        c.secType = "FUT";
        c.exchange = exchange;
        c.currency = currency;
        c.lastTradeDateOrContractMonth = expiry;
        return c;
    }

    // Criptomonedas 24/7 (PAXOS)
    [[nodiscard]] static Contract makeCrypto(const std::string_view &symbol = "BTC",
                                             const std::string_view &exchange = "PAXOS",
                                             const std::string_view &currency = "USD") {
        Contract c;
        c.symbol = symbol;
        c.secType = "CRYPTO";
        c.exchange = exchange;
        c.currency = currency;
        return c;
    }

    // Forex Spot (IDEALPRO)
    [[nodiscard]] static Contract makeForex(const std::string_view &base = "EUR",
                                            const std::string_view &quote = "USD",
                                            const std::string_view &exchange = "IDEALPRO") {
        Contract c;
        c.symbol = base;
        c.secType = "CASH";
        c.exchange = exchange;
        c.currency = quote;
        return c;
    }

    // Acciones / ETFs (SMART Routing)
    [[nodiscard]] static Contract makeStock(const std::string_view &symbol = "AAPL",
                                            const std::string_view &exchange = "SMART",
                                            const std::string_view &currency = "USD") {
        Contract c;
        c.symbol = symbol;
        c.secType = "STK";
        c.exchange = exchange;
        c.currency = currency;
        return c;
    }

    [[nodiscard]] static std::string dumpContract(const Contract &c) {
        if (!c.lastTradeDateOrContractMonth.empty()) {
            return std::format("[{}] {} (Exp: {}) @ {} ({})", c.secType, c.symbol,
                               c.lastTradeDateOrContractMonth, c.exchange, c.currency);
        }
        return std::format("[{}] {} @ {} ({})", c.secType, c.symbol, c.exchange, c.currency);
    }
};