#ifndef EXTRACT_INSCRIPTION_DATA_H
#define EXTRACT_INSCRIPTION_DATA_H

#include "univalue.h"
#include <vector>
#include <script/script.h>
#include <script/interpreter.h>
#include <primitives/transaction.h>

// Custom error type similar to SequencerError in Rust
class SequencerError : public std::runtime_error {
public:
    explicit SequencerError(const std::string& msg) : std::runtime_error(msg) {}
};

// Declaration of the function
std::vector<unsigned char> extract_inscription_data(const UniValue tx);

#endif // EXTRACT_INSCRIPTION_DATA_H
