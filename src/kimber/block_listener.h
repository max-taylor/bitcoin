#ifndef L1_BLOCK_LISTENER_H
#define L1_BLOCK_LISTENER_H

#include "node/context.h"
#include <string>

// Predefined sender address to filter
extern const std::string predefinedSenderAddress;

// /**
//  * @brief Fetch the latest block hash from the Bitcoin L1 node.
//  *
//  * @param client RPC client to communicate with the Bitcoin L1 node.
//  * @return std::string Latest block hash on the Bitcoin L1 network.
//  */
// std::string GetLatestBlockHashFromL1(rpc::client& client);
//
// /**
//  * @brief Fetch the full block data from the Bitcoin L1 node.
//  *
//  * @param client RPC client to communicate with the Bitcoin L1 node.
//  * @param blockHash The hash of the block to fetch.
//  * @return rpc::client::json_value JSON object representing the block data.
//  */
// rpc::client::json_value GetBlockFromL1(rpc::client& client, const std::string& blockHash);
//
// /**
//  * @brief Check if a given transaction is from the predefined sender address.
//  *
//  * @param tx JSON object representing the transaction.
//  * @param senderAddress The address to check against.
//  * @return true If the transaction is from the predefined sender.
//  * @return false If the transaction is not from the predefined sender.
//  */
// bool IsTransactionFromPredefinedSender(const rpc::client::json_value& tx, const std::string& senderAddress);
//
// /**
//  * @brief Process a newly mined block on the Bitcoin L1 network.
//  *
//  * This function will check the block's transactions and filter out the ones
//  * that come from the predefined sender address. It will then process those
//  * transactions (e.g., validating them for the L2 state).
//  *
//  * @param client RPC client to communicate with the Bitcoin L1 node.
//  */
// void ProcessL1Block(rpc::client& client);

/**
 * @brief Start the L1 block listener that continuously polls for new blocks.
 *
 * This function will run in its own thread and will process blocks from
 * the Bitcoin L1 network by polling for the latest block and filtering
 * transactions from the predefined sender address.
 */
void StartL1BlockListener(node::NodeContext& context);

#endif  // L1_BLOCK_LISTENER_H
