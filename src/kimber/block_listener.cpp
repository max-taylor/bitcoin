#include "block_listener.h"
#include "bitcoin_rpc_client.h"

#include <iostream>
#include <thread>
#include <chrono>
#include <cstdlib>


// The main listener function that will run in its own thread
void StartL1BlockListener() {
    while (true) {
        try {
            BitcoinRPCClient client;  // Use defaults or provide custom connection details
            std::string response = client.getBestBlockHash();
            std::cout << "Response: " << response << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }

    // try {
    //     return executeRPCRequest(request);
    // } catch (const std::exception& e) {
    //     throw std::runtime_error("Failed to get best block hash: " + std::string(e.what()));
    // }
        // Polling for new blocks every 10 seconds
        std::this_thread::sleep_for(std::chrono::seconds(5));  // Adjust polling interval as needed
    }
}
