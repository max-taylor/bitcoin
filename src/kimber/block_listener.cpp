#include "block_listener.h"
#include "bitcoin_rpc_client.h"
#include "interfaces/mining.h"
#include "interfaces/types.h"
#include "node/context.h"
#include "primitives/block.h"
#include "primitives/transaction.h"
#include "support/allocators/zeroafterfree.h"
#include "univalue.h"
#include "util/overflow.h"
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
//         // if constexpr (std::is_same_v<T, TransactionSerParams>) {
//             return TX_WITH_WITNESS;
//         // }
//         // return T();
//     }
//
//     // Forward operators to underlying stream with correct transaction handling
//     template <typename T>
//     DataStreamWithParams& operator>>(T&& obj) {
//         if constexpr (std::is_same_v<std::remove_reference_t<T>, CTransaction>) {
//             UnserializeTransaction(obj, stream, TX_WITH_WITNESS);
//         } else if constexpr (std::is_same_v<std::remove_reference_t<T>, CMutableTransaction()>) {
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
namespace util {
inline void Xor(Span<std::byte> write, Span<const std::byte> key, size_t key_offset = 0)
{
    if (key.size() == 0) {
        return;
    }
    key_offset %= key.size();

    for (size_t i = 0, j = key_offset; i != write.size(); i++) {
        write[i] ^= key[j++];

        // This potentially acts on very many bytes of data, so it's
        // important that we calculate `j`, i.e. the `key` index in this
        // way instead of doing a %, which would effectively be a division
        // for each byte Xor'd -- much slower than need be.
        if (j == key.size())
            j = 0;
    }
}
} // namespace util


/** Double ended buffer combining vector and stream-like interfaces.
 *
 * >> and << read and write unformatted data using the above serialization templates.
 * Fills with data in linear time; some stringstream implementations take N^2 time.
 */
class DataStream
{
protected:
    using vector_type = SerializeData;
    vector_type vch;
    vector_type::size_type m_read_pos{0};

public:
    typedef vector_type::allocator_type   allocator_type;
    typedef vector_type::size_type        size_type;
    typedef vector_type::difference_type  difference_type;
    typedef vector_type::reference        reference;
    typedef vector_type::const_reference  const_reference;
    typedef vector_type::value_type       value_type;
    typedef vector_type::iterator         iterator;
    typedef vector_type::const_iterator   const_iterator;
    typedef vector_type::reverse_iterator reverse_iterator;

    explicit DataStream() = default;
    explicit DataStream(Span<const uint8_t> sp) : DataStream{AsBytes(sp)} {}
    explicit DataStream(Span<const value_type> sp) : vch(sp.data(), sp.data() + sp.size()) {}

    std::string str() const
    {
        return std::string{UCharCast(data()), UCharCast(data() + size())};
    }


    //
    // Vector subset
    //
    const_iterator begin() const                     { return vch.begin() + m_read_pos; }
    iterator begin()                                 { return vch.begin() + m_read_pos; }
    const_iterator end() const                       { return vch.end(); }
    iterator end()                                   { return vch.end(); }
    size_type size() const                           { return vch.size() - m_read_pos; }
    bool empty() const                               { return vch.size() == m_read_pos; }
    void resize(size_type n, value_type c = value_type{}) { vch.resize(n + m_read_pos, c); }
    void reserve(size_type n)                        { vch.reserve(n + m_read_pos); }
    const_reference operator[](size_type pos) const  { return vch[pos + m_read_pos]; }
    reference operator[](size_type pos)              { return vch[pos + m_read_pos]; }
    void clear()                                     { vch.clear(); m_read_pos = 0; }
    value_type* data()                               { return vch.data() + m_read_pos; }
    const value_type* data() const                   { return vch.data() + m_read_pos; }

    inline void Compact()
    {
        vch.erase(vch.begin(), vch.begin() + m_read_pos);
        m_read_pos = 0;
    }


    template<typename T>
    TransactionSerParams GetParams() const
    {
        return TX_WITH_WITNESS;
    }

    bool Rewind(std::optional<size_type> n = std::nullopt)
    {
        // Total rewind if no size is passed
        if (!n) {
            m_read_pos = 0;
            return true;
        }
        // Rewind by n characters if the buffer hasn't been compacted yet
        if (*n > m_read_pos)
            return false;
        m_read_pos -= *n;
        return true;
    }


    //
    // Stream subset
    //
    bool eof() const             { return size() == 0; }
    int in_avail() const         { return size(); }

    void read(Span<value_type> dst)
    {
        if (dst.size() == 0) return;

        // Read from the beginning of the buffer
        auto next_read_pos{CheckedAdd(m_read_pos, dst.size())};
        if (!next_read_pos.has_value() || next_read_pos.value() > vch.size()) {
            throw std::ios_base::failure("DataStream::read(): end of data");
        }
        memcpy(dst.data(), &vch[m_read_pos], dst.size());
        if (next_read_pos.value() == vch.size()) {
            m_read_pos = 0;
            vch.clear();
            return;
        }
        m_read_pos = next_read_pos.value();
    }

    void ignore(size_t num_ignore)
    {
        // Ignore from the beginning of the buffer
        auto next_read_pos{CheckedAdd(m_read_pos, num_ignore)};
        if (!next_read_pos.has_value() || next_read_pos.value() > vch.size()) {
            throw std::ios_base::failure("DataStream::ignore(): end of data");
        }
        if (next_read_pos.value() == vch.size()) {
            m_read_pos = 0;
            vch.clear();
            return;
        }
        m_read_pos = next_read_pos.value();
    }

    void write(Span<const value_type> src)
    {
        // Write to the end of the buffer
        vch.insert(vch.end(), src.begin(), src.end());
    }

    template<typename T>
    DataStream& operator<<(const T& obj)
    {
        ::Serialize(*this, obj);
        return (*this);
    }

    template<typename T>
    DataStream& operator>>(T&& obj)
    {
        ::Unserialize(*this, obj);
        return (*this);
    }

    /**
     * XOR the contents of this stream with a certain key.
     *
     * @param[in] key    The key used to XOR the data in this stream.
     */
    void Xor(const std::vector<unsigned char>& key)
    {
        util::Xor(MakeWritableByteSpan(*this), MakeByteSpan(key));
    }

    /** Compute total memory usage of this object (own memory + any dynamic memory). */
    size_t GetMemoryUsage() const noexcept;
};

// CTransaction ParseTransaction(const std::vector<unsigned char>& rawTxData) {
//     DataStream stream(rawTxData);
//     CTransaction tx;
//     stream >> tx;  // Deserialize the transaction from the raw data
//     return tx;
// }
//

CBlock ConstructBlockTemplate(const std::vector<unsigned char> data) {
    // Deserialize the raw data into a Block object
    DataStream stream(data);
    // DataStreamWithParams streamWithParams(stream);
    CBlock block;

    stream >> block;

    return block;
}

std::string genesis_block = "0100000000000000000000000000000000000000000000000000000000000000000000003ba3edfd7a7b12b27ac72c3e67768f617fc81bc3888a51323a9fb8aa4b1e5e4a29ab5f49ffff001d1dac2b7c0101000000010000000000000000000000000000000000000000000000000000000000000000ffffffff4d04ffff001d0104455468652054696d65732030332f4a616e2f32303039204368616e63656c6c6f72206f6e206272696e6b206f66207365636f6e64206261696c6f757420666f722062616e6b73ffffffff0100f2052a01000000434104678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5fac00000000";

std::string other_test_string = "00000020c8c2b0fad9db29b713b18fbb1ef175d2e7e9322e25ff924039a7bd1f3c87fc40255463110a1a5e9d7e88a5b2cf93cf9a41e6a6735857b7735cc9ea8434d736d507993a67ffff7f2001000000010200000001e20409a40acfe63916ca762746110200900498aadd6057926ce1583d91bbc8c20000000000ffffffff0100000000000000000000000000";

void handle_reading_transaction(BitcoinRPCClient& client) {
    std::optional<CTransaction> transaction = client.getSequencerTransactionFromLatestBlock();

    // std::vector<unsigned char> rawTxData = ParseHex(other_test_string);
    //
    // CBlock block = ConstructBlockTemplate(rawTxData);
    //
    // std::cout << "Block header: " << block.ToString() << std::endl;

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
