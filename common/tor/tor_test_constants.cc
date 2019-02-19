/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/common/tor/tor_test_constants.h"

#define FPL FILE_PATH_LITERAL

namespace tor {

const char kTestTorProxy[] = "socks5://127.0.0.1:9999";
const char kTestTorPacString[] = "SOCKS5 127.0.0.1:9999";
const base::FilePath::CharType kTestTorPath[] = FPL(".");
const base::FilePath::CharType kTestBrokenTorPath[] = FPL("");

}  // namespace tor
