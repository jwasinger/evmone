#include <iostream>
#include <iomanip>

void print_padded_hex_le(uint64_t num) {
    //uint64_t swapped = __builtin_bswap64(num);
    std::cout << std::hex << std::setw(16) << std::setfill('0') << num;
}

// print a little-endian variable-length evm384 word as a big-endian hex number
void print_number(uint64_t *limbs, size_t num_limbs) {
    for (size_t i = num_limbs - 1; ; i--) {
        print_padded_hex_le(limbs[i]);

        if (i == 0) {
            break;
        }
    }
}
