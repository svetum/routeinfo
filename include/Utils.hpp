#ifndef UTILS
#define UTILS

inline std::string toHexFormat(uint8_t value) {
	std::string result(3, ' ');
	const char letters[] = "0123456789ABCDEF";
	char* current_hex_char = &result[0];
	*current_hex_char++ = letters[value >> 4];
	*current_hex_char++ = letters[value & 0xf];
	return result;
}

#endif
