#include "block_listener.h"
#include "bitcoin_rpc_client.h"
#include "interfaces/mining.h"
#include "node/context.h"
#include "primitives/block.h"
#include "primitives/transaction.h"
#include "streams.h"
#include "utils/sequencer_transaction_parser.h"
#include <iostream>
#include <optional>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <vector>
#include "primitives/block.h"   // Include the necessary Bitcoin Core headers

using node::NodeContext;

CBlock ConstructBlockTemplate(const std::vector<unsigned char> data) {
    // Deserialize the raw data into a Block object
    DataStream stream(data);
    CBlock block;

    stream >> block;

    return block;
}

void handle_reading_transaction(BitcoinRPCClient& client) {
    std::optional<CTransaction> transaction = client.getSequencerTransactionFromLatestBlock();

    if (!transaction.has_value()) {
        std::cout << "No transactions found in the latest block." << std::endl;

        return;
    } else {
        std::cout << "Found transaction: " << transaction.value().ToString() << std::endl;
    }

    try {
        std::cout << "Extracting inscription data from transaction..." << std::endl;

        std::vector<unsigned char> inscription_data = extract_inscription_data(transaction.value());
        std::cout << "Inscription data: " << std::string(inscription_data.begin(), inscription_data.end()) << std::endl;

        std::string value = std::string(inscription_data.begin(), inscription_data.end());
        std::vector<unsigned char> rawTxData = ParseHex(value);

        CBlock block = ConstructBlockTemplate(rawTxData);
        std::cout << "Block header: " << block.ToString() << std::endl;
        std::cout << "transactions: " << block.vtx.data()->get()->ToString() << std::endl;

    } catch (const SequencerError& e) {
        std::cerr << "Error extracting inscription data: " << e.what() << std::endl;
        return;
    }
}

// void processBlock(const CBlock& block, interfaces::Mining* mining_instance) {
//     // Process the block
//     std::cout << "Processing block: " << block.GetHash().ToString() << std::endl;
//
//     BlockValidationState state;
//
//
//     mining_instance->testBlockValidity(block, true, state);
// }

// The main listener function that will run in its own thread
void StartL1BlockListener(NodeContext& context) {
    BitcoinRPCClient client;  // Use defaults or provide custom connection details

    while (true) {

        std::this_thread::sleep_for(std::chrono::seconds(5));  // Adjust polling interval as needed
        // Access the `mining` field, which is a unique pointer
        // interfaces::Mining* mining_instance = context.mining.get();
        // std::optional<interfaces::BlockRef> block_ref = mining_instance->getTip();
        // std::cout << "Current block height: " << block_ref->height << std::endl;
        handle_reading_transaction(client);

        // Polling for new blocks every 10 seconds
    }
}
