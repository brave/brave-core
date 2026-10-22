// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ios/web/content/init/ios_content_main_runner.h"

#include "base/types/fixed_array.h"
#include "components/crash/core/common/crash_key.h"
#include "content/public/app/content_main.h"
#include "content/public/app/content_main_runner.h"
#include "ios/web/content/init/ios_main_delegate.h"
#include "ios/web/public/init/web_main.h"
#include "ios/web/public/init/web_main_delegate.h"

namespace web {
namespace {

WebMainDelegate* g_web_main_delegate = nullptr;

// Upstream drops the embedder's WebMainDelegate, which is where Brave sets up
// its command line (e.g. the component updater URL).
class BraveIOSMainDelegate : public IOSMainDelegate {
 public:
  std::optional<int> BasicStartupComplete() override {
    if (g_web_main_delegate) {
      g_web_main_delegate->BasicStartupComplete();
    }
    return IOSMainDelegate::BasicStartupComplete();
  }
};

}  // namespace
}  // namespace web

#define IOSMainDelegate BraveIOSMainDelegate
// Captures the delegate in Initialize(), the only place `params` exists.
#define args args); g_web_main_delegate = (params.delegate.get()

#include <ios/web/content/init/ios_content_main_runner.cc>

#undef args
#undef IOSMainDelegate
