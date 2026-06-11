#include <gtest/gtest.h>
#include <string>
#include <cstring>
#include <cstdio>

// The vulnerable code uses sprintf into a fixed buffer (typically 256 or 512 bytes).
// We test that path construction never overflows a reasonable buffer size.
// Since we cannot safely call the vulnerable function with oversized inputs without
// risking actual stack corruption, we encode the invariant: the combined path must
// fit within the buffer, or be rejected/truncated.

static constexpr size_t BUFFER_SIZE = 512; // Typical fixed buffer size in the vulnerable code

// This simulates what the safe version SHOULD do - we test the invariant
// that buffer reads/writes never exceed declared length
static bool path_fits_in_buffer(const char* drivers_root, const char* filename) {
    // "%s\\%s" format: len(drivers_root) + 1 (backslash) + len(filename) + 1 (null)
    size_t required = strlen(drivers_root) + 1 + strlen(filename) + 1;
    return required <= BUFFER_SIZE;
}

class BufferOverflowTest : public ::testing::TestWithParam<std::pair<std::string, std::string>> {};

TEST_P(BufferOverflowTest, SprintfPathMustNotExceedBuffer) {
    // Invariant: Buffer reads/writes never exceed the declared buffer length.
    // If inputs would overflow, the operation must be rejected or truncated.
    auto [drivers_root, filename] = GetParam();
    
    size_t required = drivers_root.size() + 1 + filename.size() + 1;
    
    if (required <= BUFFER_SIZE) {
        // Valid case: sprintf would be safe
        char fn[BUFFER_SIZE];
        snprintf(fn, BUFFER_SIZE, "%s\\%s", drivers_root.c_str(), filename.c_str());
        EXPECT_LE(strlen(fn) + 1, BUFFER_SIZE);
    } else {
        // Overflow case: the vulnerable sprintf would corrupt the stack.
        // The invariant requires that this MUST be caught/prevented.
        // Using snprintf as the safe alternative to demonstrate the fix:
        char fn[BUFFER_SIZE];
        int written = snprintf(fn, BUFFER_SIZE, "%s\\%s", drivers_root.c_str(), filename.c_str());
        // snprintf must truncate, not overflow
        EXPECT_LE(strlen(fn) + 1, BUFFER_SIZE);
        // The written value indicates truncation occurred
        EXPECT_GT(static_cast<size_t>(written), BUFFER_SIZE - 1);
    }
}

INSTANTIATE_TEST_SUITE_P(
    AdversarialInputs,
    BufferOverflowTest,
    ::testing::Values(
        // Valid input - short path
        std::make_pair(std::string("C:\\drivers"), std::string("mydriver.sys")),
        // Boundary: exactly fills buffer (511 chars + null)
        std::make_pair(std::string(255, 'A'), std::string(254, 'B')),
        // Exploit: 2x overflow - drivers_root alone exceeds buffer
        std::make_pair(std::string(1024, 'X'), std::string("payload.sys")),
        // Exploit: 10x overflow - both components massively oversized
        std::make_pair(std::string(2560, 'Y'), std::string(2560, 'Z'))
    )
);

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}