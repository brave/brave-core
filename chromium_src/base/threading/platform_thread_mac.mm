/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// Default feaeture state override requires global constructor to be available.
// Enable it where required.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wglobal-constructors"

#include "../../../../base/threading/platform_thread_mac.mm"

#pragma clang diagnostic pop
