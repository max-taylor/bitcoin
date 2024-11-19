#include "block_listener.h"
#include "bitcoin_rpc_client.h"
#include "consensus/validation.h"
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

CBlock parseInscriptionDataToBlockTemplate(const std::vector<unsigned char> data) {
    // Deserialize the raw data into a Block object
    DataStream stream(data);
    CBlock block;

    stream >> block;

    return block;
}

std::optional<CBlock> tryReadInscriptionBlockFromL1(BitcoinRPCClient& client) {
    std::optional<CTransaction> transaction = client.getSequencerTransactionFromLatestBlock();

    if (!transaction.has_value()) {
        return std::nullopt;
    }

    try {
        // std::cout << "Extracting inscription data from transaction..." << std::endl;

        std::vector<unsigned char> inscription_data = extract_inscription_data(transaction.value());
        // std::cout << "Inscription data: " << std::string(inscription_data.begin(), inscription_data.end()) << std::endl;

        std::string value = std::string(inscription_data.begin(), inscription_data.end());
        std::vector<unsigned char> rawTxData = ParseHex(value);

        CBlock block = parseInscriptionDataToBlockTemplate(rawTxData);

        return block;
    } catch (const SequencerError& e) {
        std::cerr << "Error extracting inscription data: " << e.what() << std::endl;
        return std::nullopt;
    }
}

void processBlock(const CBlock& block, interfaces::Mining* mining_instance) {
    // Process the block
    std::cout << "Processing block: " << block.GetHash().ToString() << std::endl;
    block.m_checked_witness_commitment = true;

    std::shared_ptr<const CBlock> block_ptr = std::make_shared<CBlock>(block);
    bool result = true;

    mining_instance->processNewBlock(block_ptr, &result);
}

// The main listener function that will run in its own thread
void StartL1BlockListener(NodeContext& context) {
    BitcoinRPCClient client;  // Use defaults or provide custom connection details

    while (true) {
        // TODO: Should be updated to detect new Bitcoin blocks
        std::this_thread::sleep_for(std::chrono::seconds(5));

        std::optional<CBlock> inscriptionBlock = tryReadInscriptionBlockFromL1(client);

        if (!inscriptionBlock.has_value()) {
            std::cout << "No transactions found in the latest block." << std::endl;

            continue;
        }

        processBlock(inscriptionBlock.value(), context.mining.get());
    }
}
