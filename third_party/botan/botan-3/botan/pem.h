/*
* PEM Encoding/Decoding
* (C) 1999-2007 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_PEM_H_
#define BOTAN_PEM_H_

#include <botan/secmem.h>
#include <string>
#include <string_view>

namespace Botan {

class DataSource;

namespace PEM_Code {

/**
* Encode some binary data in PEM format
* @param data binary data to encode
* @param data_len length of binary data in bytes
* @param label PEM label put after BEGIN and END
* @param line_width after this many characters, a new line is inserted
*/
BOTAN_PUBLIC_API(2, 0)
std::string encode(const uint8_t data[], size_t data_len, std::string_view label, size_t line_width = 64);

/**
* Encode some binary data in PEM format
* @param data binary data to encode
* @param label PEM label
* @param line_width after this many characters, a new line is inserted
*/
template <typename Alloc>
std::string encode(const std::vector<uint8_t, Alloc>& data, std::string_view label, size_t line_width = 64) {
   return encode(data.data(), data.size(), label, line_width);
}

/**
* Decode PEM data
* @param pem a datasource containing PEM encoded data
* @param label is set to the PEM label found for later inspection
*/
BOTAN_PUBLIC_API(2, 0) secure_vector<uint8_t> decode(DataSource& pem, std::string& label);

/**
* Decode PEM data
* @param pem a string containing PEM encoded data
* @param label is set to the PEM label found for later inspection
*/
BOTAN_PUBLIC_API(2, 0) secure_vector<uint8_t> decode(std::string_view pem, std::string& label);

/**
* Decode PEM data
* @param pem a datasource containing PEM encoded data
* @param label is what we expect the label to be
*/
BOTAN_PUBLIC_API(2, 0)
secure_vector<uint8_t> decode_check_label(DataSource& pem, std::string_view label);

/**
* Decode PEM data
* @param pem a string containing PEM encoded data
* @param label is what we expect the label to be
*/
BOTAN_PUBLIC_API(2, 0)
secure_vector<uint8_t> decode_check_label(std::string_view pem, std::string_view label);

/**
* Heuristic test for PEM data.
*/
BOTAN_PUBLIC_API(2, 0) bool matches(DataSource& source, std::string_view extra = "", size_t search_range = 4096);

}  // namespace PEM_Code

}  // namespace Botan

#endif
