#include "block_listener.h"
#include "bitcoin_rpc_client.h"
#include "univalue.h"
#include "utils/sequencer_transaction_parser.h"
#include <iostream>
#include <optional>
#include <thread>
#include <chrono>
#include <cstdlib>

void handle_reading_transaction(BitcoinRPCClient& client) {
    std::optional<UniValue> transaction = client.getSequencerTransactionFromLatestBlock();

    if (!transaction.has_value()) {
        std::cout << "No transactions found in the latest block." << std::endl;

        return;
    } else {
        std::cout << "Found transaction: " << transaction.value().write() << std::endl;
    }

    try {
        std::cout << "Extracting inscription data from transaction..." << std::endl;

        std::vector<unsigned char> inscription_data = extract_inscription_data(transaction.value());
        std::cout << "Inscription data: " << std::string(inscription_data.begin(), inscription_data.end()) << std::endl;
    } catch (const SequencerError& e) {
        std::cerr << "Error extracting inscription data: " << e.what() << std::endl;
        return;
    }
}

// The main listener function that will run in its own thread
void StartL1BlockListener() {
    BitcoinRPCClient client;  // Use defaults or provide custom connection details

    while (true) {
        handle_reading_transaction(client);

        // Polling for new blocks every 10 seconds
        std::this_thread::sleep_for(std::chrono::seconds(5));  // Adjust polling interval as needed
    }
}
