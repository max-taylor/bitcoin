#ifndef BITCOIN_RPC_CLIENT_H
#define BITCOIN_RPC_CLIENT_H

#include "primitives/transaction.h"
#include <univalue.h>
#include <string>

class BitcoinRPCClient {
public:
    /**
     * @brief Constructs a Bitcoin RPC client
     * @param host The hostname where the Bitcoin node is running
     * @param port The RPC port number
     * @param username The RPC authentication username
     * @param password The RPC authentication password
     */
    BitcoinRPCClient(
        const std::string& host = "127.0.0.1",
        const int port = 8333,
        const std::string& username = "admin",
        const std::string& password = "admin"
    );

    /**
     * @brief Executes a JSON-RPC request to the Bitcoin node
     * @param jsonRequest The JSON-RPC request string
     * @return The response from the Bitcoin node
     * @throws std::runtime_error if the command execution fails
     */
    std::string executeRPCRequest(const std::string& jsonRequest);

    /**
     * @brief Gets the hash of the best block in the chain
     * @return The best block hash as a string
     * @throws std::runtime_error if the request fails
     */
    std::string getBestBlockHash();

    /**
     * @brief Gets the hash of a block at a given height
     * @param height The height of the block
     * @return The block hash as a string
     * @throws std::runtime_error if the request fails
     */
    std::string getBlockHashByHeight(int height);


    /**
     * @brief Gets the block data for a given block hash
     * @param blockHash The hash of the block
     * @return The block data as a string
     * @throws std::runtime_error if the request fails
     */
    std::string getBlockByHash(const std::string& blockHash);


    /**
     * @brief Tries to get a sequencer transaction from the latest block
     * @return A UniValue object representing the transaction
     * @throws std::runtime_error if the request fails
     */
    std::optional<CTransaction> getSequencerTransactionFromLatestBlock();

private:
    std::string host;
    int port;
    std::string username;
    std::string password;

    /**
     * @brief Helper function to execute a shell command and get output
     * @param cmd The shell command to execute
     * @return The command's output as a string
     * @throws std::runtime_error if command execution fails
     */
    std::string execCommand(const std::string& cmd);
};

#endif // BITCOIN_RPC_CLIENT_H
