/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include<string>
#include <iostream>
#include <regex>

#include <algorithm>
#include <string>

#include "base/strings/string_tokenizer.h"
#include "utils.h"

std::string convert_to_str(const uint8_t* ptr, int size) {
  std::string str;
  for (int i = 0; i < size; i++) {
    str.append(std::to_string(int(ptr[i])));
    if (i != size - 1) {
      str.append(", ");
    }
  }
  str.insert(0, "[");
  str.append("]");
  return str;
}

void parse_str_response(const char* ptr, uint8_t* dst) {
  std::string str(ptr);

  // remove parenthesis 
  str.pop_back();
  str.erase(str.begin());
  const char* str_ptr = str.c_str(); 

  base::CStringTokenizer token(str_ptr, str_ptr + std::strlen(str_ptr), ", ");
  uint i = 0;
  while (token.GetNext()) {
    dst[i] = std::stoi(token.token().c_str());
    i++;
  }
}

int get_size_response(const char* ptr) {
  std::string str(ptr);
  const char* str_ptr = str.c_str(); 
  base::CStringTokenizer token(str_ptr, str_ptr + std::strlen(str_ptr), ", ");
  int i = 0;
  while (token.GetNext()) {
    i++;
  }
  return i;
}