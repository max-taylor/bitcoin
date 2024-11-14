#include "bitcoin_rpc_client.h"
#include <cstdio>
#include <iostream>
#include <ostream>

BitcoinRPCClient::BitcoinRPCClient(
    const std::string& host_,
    const int port_,
    const std::string& username_,
    const std::string& password_
) : host(host_), port(port_), username(username_), password(password_) {}

std::string BitcoinRPCClient::execCommand(const std::string& cmd) {
    std::array<char, 128> buffer;
    std::string result;

    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }

    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }

    return result;
}

std::string BitcoinRPCClient::executeRPCRequest(const std::string& jsonRequest) {
    // Construct curl command - note we don't need to escape the quotes in jsonRequest
    // because we're wrapping it in single quotes
    std::string cmd = "curl --user " + username + ":" + password +
                     " --data-binary '" + jsonRequest + "'" +
                     " -H 'content-type: application/json'" +
                     " http://" + host + ":" + std::to_string(port);

    // Log cmd
    std::cout << "Executing command: " << cmd << std::endl;

    // Execute command and return response
    return execCommand(cmd);
}

std::string BitcoinRPCClient::getBestBlockHash() {
    std::string request = "{\"jsonrpc\": \"2.0\", \"id\": \"63\", \"method\": \"getbestblockhash\", \"params\": []}";

    try {
        return executeRPCRequest(request);
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to get best block hash: " + std::string(e.what()));
    }
}
