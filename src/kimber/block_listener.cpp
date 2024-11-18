#include "block_listener.h"
#include "bitcoin_rpc_client.h"
#include "interfaces/mining.h"
#include "interfaces/types.h"
#include "node/context.h"
#include "primitives/block.h"
#include "primitives/transaction.h"
#include "streams.h"
#include "univalue.h"
#include "utils/sequencer_transaction_parser.h"
#include <iostream>
#include <optional>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <vector>
#include "primitives/block.h"   // Include the necessary Bitcoin Core headers
#include "serialize.h"           // For serialization

using node::NodeContext;

// class DataStreamWithParams {
// private:
//     DataStream& stream;
// public:
//     DataStreamWithParams(DataStream& s) : stream(s) {}
//
//     template<typename T>
//     static T GetParams() {
//         if constexpr (std::is_same_v<T, TransactionSerParams>) {
//             return TX_WITH_WITNESS;
//         }
//         return T();
//     }
//
//     // Forward operators to underlying stream with correct transaction handling
//     template <typename T>
//     DataStreamWithParams& operator>>(T&& obj) {
//         if constexpr (std::is_same_v<std::remove_reference_t<T>, CMutableTransaction>) {
//             UnserializeTransaction(obj, stream, TX_WITH_WITNESS);
//         } else if constexpr (std::is_same_v<std::remove_reference_t<T>, CBlock>) {
//             // Use the block's own Unserialize method
//             obj.Unserialize(stream);
//
//             // If the block contains transactions, deserialize them with witness support
//             for (auto& tx : obj.vtx) {
//                 if (tx) {
//                     UnserializeTransaction(*tx, stream, TX_WITH_WITNESS);
//                 }
//             }
//         } else {
//             stream >> std::forward<T>(obj);
//         }
//         return *this;
//     }
//
//     // Forward other methods
//     size_t size() const { return stream.size(); }
//     bool eof() const { return stream.eof(); }
//     void read(Span<DataStream::value_type> dst) { stream.read(dst); }
//     void write(Span<const DataStream::value_type> src) { stream.write(src); }
// };

// CTransaction ParseTransaction(const std::vector<unsigned char>& rawTxData) {
//     DataStream stream(rawTxData);
//     CTransaction tx;
//     stream >> tx;  // Deserialize the transaction from the raw data
//     return tx;
// }
//

// CBlock ConstructBlockTemplate(const std::vector<unsigned char> data) {
//     // Deserialize the raw data into a Block object
//     DataStream stream(data);
//     // DataStreamWithParams streamWithParams(stream);
//     CBlock block;
//     CMutableTransaction tx;
//     UnserializeTransaction(tx, stream, TX_WITH_WITNESS);
//
//     // streamWithParams >> block;
//
//     std::cout << "Block header: " << block.ToString() << std::endl;
//     return block;
// }

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

        CBlock block;
        // CBlock block = ConstructBlockTemplate(inscription_data);
    } catch (const SequencerError& e) {
        std::cerr << "Error extracting inscription data: " << e.what() << std::endl;
        return;
    }
}

// The main listener function that will run in its own thread
void StartL1BlockListener(NodeContext& context) {
    BitcoinRPCClient client;  // Use defaults or provide custom connection details

    while (true) {
        // Access the `mining` field, which is a unique pointer
        // interfaces::Mining* mining_instance = context.mining.get();
        // std::optional<interfaces::BlockRef> block_ref = mining_instance->getTip();
        // std::cout << "Current block height: " << block_ref->height << std::endl;
        handle_reading_transaction(client);

        // Polling for new blocks every 10 seconds
        std::this_thread::sleep_for(std::chrono::seconds(5));  // Adjust polling interval as needed
    }
}
