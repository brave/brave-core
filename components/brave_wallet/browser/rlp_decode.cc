/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/rlp_decode.h"

#include <utility>

namespace {

// Decodes an integer
bool RLPToInteger(const std::string& s, size_t* val) {
  size_t length = s.length();
  if (length == 0) {
    return false;
  }

  if (length == 1) {
    *val = static_cast<size_t>(static_cast<uint8_t>(s[0]));
    return true;
  }

  size_t v2;
  if (length <= 1 || !RLPToInteger(s.substr(0, length - 1), &v2)) {
    return false;
  }
  *val = static_cast<size_t>(static_cast<uint8_t>(s[length - 1]) + v2 * 256);
  return true;
}

bool IsWithinBounds(size_t offset, size_t data_len, size_t length) {
  // This seems redundant but it is resistant to overflows
  return offset <= length && data_len <= length && offset + data_len <= length;
}

// Decodes an offset, length, and value
bool RLPDecodeLength(const std::string& s,
                     size_t* offset,
                     size_t* data_len,
                     base::Value* value) {
  size_t length = s.length();
  if (length == 0) {
    return false;
  }
  uint8_t prefix = static_cast<uint8_t>(s[0]);
  if (prefix <= 0x7f) {
    *offset = 0;
    *data_len = 1;
    std::string str = {s[0]};
    *value = base::Value(str);
    return true;
  }

  if (prefix <= 0xb7 && length > prefix - 0x80) {
    size_t strLen = prefix - 0x80;
    *offset = 1;
    *data_len = strLen;
    if (!IsWithinBounds(*offset, *data_len, length)) {
      return false;
    }
    // If a string length is 1 it should have been handled by the single byte
    // clause above.
    if (*data_len == 1) {
      return false;
    }
    std::string str = s.substr(*offset, *data_len);
    *value = base::Value(str);
    return true;
  }

  size_t i;
  if (1 + prefix - 0xb7 <= length &&
      RLPToInteger(s.substr(1, prefix - 0xb7), &i)) {
    if (prefix <= 0xbf && length > prefix - 0xb7 &&
        length > prefix - 0xb7 + i) {
      size_t strLen = i;
      *offset = 1 + prefix - 0xb7;
      *data_len = strLen;
      if (!IsWithinBounds(*offset, *data_len, length)) {
        return false;
      }
      // If a list contains 0-55 bytes, it should have been handled above by
      // the RLP encoding spec.  So this input should never happen, even though
      // it could in theory decode properly.
      if (*data_len <= 55) {
        return false;
      }

      std::string str = s.substr(*offset, *data_len);
      *value = base::Value(str);
      return true;
    }
  }

  if (prefix <= 0xf7 && length > prefix - 0xc0) {
    *offset = 1;
    *data_len = prefix - 0xc0;
    *value = base::ListValue();
    return true;
  }

  // The data is a list if the range of the first byte is [0xf8, 0xff], and the
  // total payload of the list whose length is equal to the first byte minus
  // 0xf7 follows the first byte, and the concatenation of the RLP encodings
  // of all items of the list follows the total payload of the list;
  size_t list_data_len;
  size_t list_len_length = prefix - 0xf7;
  if (prefix <= 0xff && length >= 1 + prefix - 0xf7 &&
      RLPToInteger(s.substr(1, list_len_length), &list_data_len)) {
    // Skip past the prefix and the list len length
    *offset = 1 + list_len_length;
    *data_len = list_data_len;
    // If a list contains 0-55 elements, it should have been handled above by
    // the RLP encoding spec.  So this input should never happen, even though
    // it could in theory decode properly.
    if (*data_len <= 55 || !IsWithinBounds(*offset, *data_len, length)) {
      return false;
    }
    *value = base::ListValue();
    return true;
  }

  return false;
}

// Decodes a string and gives the result, an offset, and a data_len
bool RLPDecodeInternal(const std::string& s,
                       base::Value* output,
                       size_t* offset,
                       size_t* data_len) {
  size_t length = s.length();
  if (!RLPDecodeLength(s, offset, data_len, output)) {
    return false;
  }

  if (output->is_string()) {
    if (!IsWithinBounds(*offset, *data_len, length)) {
      return false;
    }
    std::string str = s.substr(*offset, *data_len);
    *output = base::Value(str);
  } else if (output->is_list()) {
    *output = base::ListValue();
    base::ListValue* output_list;
    if (!output->GetAsList(&output_list)) {
      return false;
    }
    if (!IsWithinBounds(*offset, *data_len, length)) {
      return false;
    }
    std::string sub = s.substr(*offset, *data_len);
    while (sub.length() > 0) {
      base::Value v;
      size_t offset2, data_len2;
      if (!RLPDecodeInternal(sub, &v, &offset2, &data_len2)) {
        return false;
      }
      output_list->Append(std::move(v));
      *offset += data_len2 + offset2;
      *data_len -= data_len2 + offset2;
      if (!IsWithinBounds(*offset, *data_len, length)) {
        return false;
      }
      sub = s.substr(*offset, *data_len);
    }
  }

  return true;
}

}  // namespace

namespace brave_wallet {

bool RLPDecode(const std::string& s, base::Value* output) {
  if (!output) {
    return false;
  }
  size_t offset;
  size_t data_len;
  bool result = RLPDecodeInternal(s, output, &offset, &data_len);
  if (!result) {
    *output = base::Value();
  }
  return result;
}

}  // namespace brave_wallet
