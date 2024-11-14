#include "bitcoin_rpc_client.h"
#include <univalue.h>
#include <cstdio>
#include <iostream>
#include <ostream>

// Predefined sender address to filter
// const std::string predefinedSenderAddress = "yourSenderAddressHere";

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

    return executeRPCRequest(request);
}


std::string BitcoinRPCClient::getBlockHashByHeight(int height) {
    std::string request = "{\"jsonrpc\": \"2.0\", \"id\": \"64\", \"method\": \"getblockhash\", \"params\": [" + std::to_string(height) + "]}";
    return executeRPCRequest(request);
}

std::string BitcoinRPCClient::getBlockByHash(const std::string& blockHash) {
    std::string request = "{\"jsonrpc\": \"2.0\", \"id\": \"65\", \"method\": \"getblock\", \"params\": [\"" + blockHash + "\", 2]}";
    return executeRPCRequest(request);
}
//
// std::vector<UniValue> BitcoinRPCClient::getTransactionsFromLatestBlock() {
//     // Step 1: Get the hash of the latest block
//     std::string latestBlockHashResponse = getBestBlockHash();
//
//     // Parse the response to extract the block hash
//     UniValue hashResponse;
//     if (!hashResponse.read(latestBlockHashResponse) || !hashResponse["result"].isStr()) {
//         throw std::runtime_error("Failed to parse latest block hash from response");
//     }
//     std::string latestBlockHash = hashResponse["result"].get_str();
//
//     // Step 2: Get the latest block details, including transactions
//     std::string blockDataJson = getBlockByHash(latestBlockHash);
//
//     // Parse the block JSON to access transactions
//     UniValue blockData;
//     if (!blockData.read(blockDataJson) || !blockData["result"].isObject()) {
//         throw std::runtime_error("Failed to parse block data JSON");
//     }
//
//     const UniValue& transactions = blockData["result"]["tx"];
//     if (!transactions.isArray()) {
//         throw std::runtime_error("Transaction data is not in the expected array format");
//     }
//
//     // Step 3: Filter transactions by the predefined sender address
//     std::vector<UniValue> filteredTransactions;
//     for (size_t i = 0; i < transactions.size(); ++i) {
//         const UniValue& transaction = transactions[i];
//         // const UniValue& inputs = transaction["vin"];
//
//         // Log transaction
//         std::cout << "Transaction: " << transaction.write() << std::endl;
//
//         // // Check each input in the transaction for the predefined sender address
//         // for (size_t j = 0; j < inputs.size(); ++j) {
//         //     const UniValue& input = inputs[j];
//         //     if (input.exists("prevout") && input["prevout"].exists("addresses")) {
//         //         const UniValue& addresses = input["prevout"]["addresses"];
//         //         for (size_t k = 0; k < addresses.size(); ++k) {
//         //             if (addresses[k].isStr() && addresses[k].get_str() == predefinedSenderAddress) {
//         //                 filteredTransactions.push_back(transaction);
//         //                 break;
//         //             }
//         //         }
//         //     }
//         // }
//     }
//     return filteredTransactions;
// }
