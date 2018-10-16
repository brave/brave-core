// Copyright 2018 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "../../../../content/browser/browser_context.cc"

namespace content {

bool BrowserContext::IsTorProfile() const {
  return false;
}

}  // namespace content
