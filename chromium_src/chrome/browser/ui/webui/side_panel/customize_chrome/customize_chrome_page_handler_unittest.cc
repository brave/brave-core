// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "content/public/browser/per_web_ui_browser_interface_broker.h"
#include "mojo/public/cpp/bindings/receiver.h"

// Add MOCK_METHOD to MockPage class
#define BindNewPipeAndPassRemote()                                     \
  BindNewPipeAndPassRemote();                                          \
  }                                                                    \
  MOCK_METHOD(void, OnUseDarkerThemeChanged, (bool use_darker_theme)); \
  void UnusedMethod() {
#include <chrome/browser/ui/webui/side_panel/customize_chrome/customize_chrome_page_handler_unittest.cc>
#undef BindNewPipeAndPassRemote
