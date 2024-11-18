#include "sequencer_transaction_parser.h"
#include <univalue.h>
#include <primitives/transaction.h> // For CTransaction
#include <script/script.h>           // For CScript and opcodes
#include <vector>

std::vector<unsigned char> extract_inscription_data(const CTransaction& tx) {
    // Ensure there is at least one input in the transaction
    if (tx.vin.empty()) {
        throw SequencerError("No input found in transaction");
    }

    // Get the script witness for the first input
    const CScriptWitness& witness = tx.vin[0].scriptWitness;
    if (witness.stack.size() < 2) {
        throw SequencerError("No inscription script found in witness");
    }

    // The inscription script is the second item in the witness stack
    const std::vector<unsigned char>& inscription_script = witness.stack[1];

    // Create a CScript instance from the inscription script bytes
    CScript script(inscription_script.begin(), inscription_script.end());

    std::vector<unsigned char> data;
    bool in_envelope = false;

    // Iterate through script instructions
    CScript::const_iterator it = script.begin();
    opcodetype opcode;
    std::vector<unsigned char> pushData;
    while (script.GetOp(it, opcode, pushData)) {
        if (opcode == OP_IF) {
            in_envelope = true;
        } else if (opcode == OP_ENDIF) {
            in_envelope = false;
        } else if (in_envelope && opcode <= OP_PUSHDATA4) {
            // Collect push data while in envelope
            data.insert(data.end(), pushData.begin(), pushData.end());
        }
    }

    return data;
}
