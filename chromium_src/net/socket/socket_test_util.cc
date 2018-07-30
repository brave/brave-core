// Copyright 2018 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "../../../../net/socket/socket_test_util.cc"

namespace net {

const char kSOCKS5GreetRequestAuth[] = { 0x05, 0x01, 0x02 };
const int kSOCKS5GreetRequestAuthLength = arraysize(kSOCKS5GreetRequestAuth);

const char kSOCKS5GreetResponseAuth[] = { 0x05, 0x02 };
const int kSOCKS5GreetResponseAuthLength = arraysize(kSOCKS5GreetResponseAuth);

}  // namespace net
