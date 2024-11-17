#include "sequencer_transaction_parser.h"
#include <univalue.h>
#include <primitives/transaction.h> // For CTransaction
#include <script/script.h>           // For CScript and opcodes
#include <vector>

std::vector<unsigned char> extract_inscription_data(UniValue txJson) {
    // Ensure there is at least one input in the transaction
    if (!txJson.exists("vin") || !txJson["vin"].isArray() || txJson["vin"].size() == 0) {
        throw SequencerError("No input found in transaction");
    }

    // Get the first input's script witness
    const UniValue& input = txJson["vin"][0];
    if (!input.exists("txinwitness") || !input["txinwitness"].isArray() || input["txinwitness"].size() < 2) {
        throw SequencerError("No inscription script found in witness");
    }

    const UniValue& witnessStack = input["txinwitness"];
    const std::string inscriptionScriptHex = witnessStack[1].get_str();  // The second item in the stack is the inscription script

    // Convert the hex string to a vector of bytes
    std::vector<unsigned char> inscriptionScript = ParseHex(inscriptionScriptHex);

    // Create a CScript instance from the inscription script bytes
    CScript script(inscriptionScript.begin(), inscriptionScript.end());

    std::vector<unsigned char> data;
    bool inEnvelope = false;

    // Iterate through script instructions
    CScript::const_iterator it = script.begin();
    opcodetype opcode;
    std::vector<unsigned char> pushData;
    while (script.GetOp(it, opcode, pushData)) {
        if (opcode == OP_IF) {
            inEnvelope = true;
        } else if (opcode == OP_ENDIF) {
            inEnvelope = false;
        } else if (inEnvelope && opcode <= OP_PUSHDATA4) {
            // Collect push data while in envelope
            data.insert(data.end(), pushData.begin(), pushData.end());
        }
    }

    return data;
}

