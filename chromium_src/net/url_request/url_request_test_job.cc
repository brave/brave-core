/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// When building for iOS with XCode 11.3.1 the build fails because Erase is
// declared and defined earlier than EraseIf that's used in Erase. Putting a
// forward declaration here helps get around that. XCode 11.4.1 doesn't have
// this problem.
#include <list>

namespace base {
template <class T, class Allocator, class Predicate>
size_t EraseIf(std::list<T, Allocator>& container, Predicate pred);  // NOLINT
}  // namespace base
#include "base/stl_util.h"

#include "src/net/url_request/url_request_test_job.cc"
