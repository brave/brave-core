/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/json/json_reader.h"

#include "services/data_decoder/public/cpp/data_decoder.h"

#define JSON_PARSE_RFC JSON_PARSE_RFC | base::JSON_ALLOW_TRAILING_COMMAS
#include "src/services/data_decoder/public/cpp/data_decoder.cc"
#undef JSON_PARSE_RFC
