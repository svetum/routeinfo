#ifndef UTILS
#define UTILS

#include <iostream>

inline std::string to_hex(uint8_t value) 
{
	std::string result(3, ' ');
	const char letters[] = "0123456789ABCDEF";
	char* current_hex_char = &result[0];
	*current_hex_char++ = letters[value >> 4];
	*current_hex_char++ = letters[value & 0xf];
	return result;
}

inline unsigned short get_identifier() 
{
	return static_cast<unsigned short>(::getpid());
}

inline void print_message(uint8_t *message, int length) 
{
	for(int i = 0; i < length; ++i) 
	{
		std::cout << to_hex(message[i]);
		if((i +1) % 4 == 0) 
			std::cout << std::endl;
	}
}

#endif
