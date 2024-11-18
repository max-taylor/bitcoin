#include "bitcoin_rpc_client.h"
#include "primitives/transaction.h"
#include "streams.h"
#include <univalue.h>
#include <cstdio>
#include <iostream>
#include <ostream>
#include <span.h>
#include <util/strencodings.h>  // for ParseHex

// Predefined sender address to filter
const std::string predefinedSenderAddress = "bcrt1pyjqsdapf9zpy7w79j6xqf6h2jqy8de67rktppg6yevhctvyegnsqevnr2d";

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

// Helper function to convert UniValue JSON to CTransaction
CTransaction ConvertToCTransaction(const UniValue& transactionJson) {
    if (!transactionJson.exists("hex") || !transactionJson["hex"].isStr()) {
        throw std::runtime_error("Transaction hex not found in JSON data");
    }

    // Get raw transaction hex string
    std::string rawTxHex = transactionJson["hex"].get_str();

    // Convert hex to binary and deserialize into CTransaction
    std::vector<unsigned char> txData(ParseHex(rawTxHex));
    DataStream stream(txData);
    CMutableTransaction tx;

    // We have to use this method directly over >> operator because we need to pass the flag TX_WITH_WITNESS and bitcoin-core has deprecated CDataStream which would've handled this for us
    UnserializeTransaction(tx, stream, TX_WITH_WITNESS);

    return CTransaction(tx);
}

std::optional<CTransaction> BitcoinRPCClient::getSequencerTransactionFromLatestBlock() {
    // Step 1: Get the hash of the latest block
    std::string latestBlockHashResponse = getBestBlockHash();

    // Parse the response to extract the block hash
    UniValue hashResponse;
    if (!hashResponse.read(latestBlockHashResponse) || !hashResponse["result"].isStr()) {
        std ::cout << "Failed to parse latest block hash from response" << std::endl;
        return std::nullopt;
        // throw std::runtime_error("Failed to parse latest block hash from response");
    }

    std::string latestBlockHash = hashResponse["result"].get_str();

    // Step 2: Get the latest block details, including transactions
    std::string blockDataJson = getBlockByHash(latestBlockHash);

    // Parse the block JSON to access transactions
    UniValue blockData;
    if (!blockData.read(blockDataJson) || !blockData["result"].isObject()) {
        throw std::runtime_error("Failed to parse block data JSON");
    }

    const UniValue& transactions = blockData["result"]["tx"];
    if (!transactions.isArray()) {
        throw std::runtime_error("Transaction data is not in the expected array format");
    }

    std::cout << "Transactions in block: " << transactions.size() << std::endl;

    // TODO: Should handle the case where there are multiple transactions
    for (size_t i = 0; i < transactions.size(); ++i) {
        const UniValue& transaction = transactions[i];
        const UniValue& outputs = transaction["vout"];

        // Log transaction
        std::cout << "Transaction " << transaction.write() << std::endl;

        if (outputs.size() != 1) {
            // Skip transactions with more than one output
            continue;
        }

        const UniValue& output = outputs[0];

        if (!output.exists("scriptPubKey")) {
            // Skip transactions without the expected output structure
            continue;
        }

        if (output["scriptPubKey"]["address"].get_str() == predefinedSenderAddress) {
            try {
                return ConvertToCTransaction(transaction);
            } catch (const std::exception& e) {
                std::cout << "Failed to convert transaction: " << e.what() << std::endl;
                continue; // Continue if conversion fails
            }
        }
    }

    return std::nullopt;
}
