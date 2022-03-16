/* Copyright (c) 2018 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_DEVTOOLS_BRAVE_DEVTOOLS_UI_BINDINGS_H_
#define BRAVE_BROWSER_DEVTOOLS_BRAVE_DEVTOOLS_UI_BINDINGS_H_

#include "chrome/browser/devtools/devtools_ui_bindings.h"

class BraveDevToolsUIBindings : public DevToolsUIBindings {
 public:
  using DevToolsUIBindings::DevToolsUIBindings;

  BraveDevToolsUIBindings(const BraveDevToolsUIBindings&) = delete;
  BraveDevToolsUIBindings& operator=(const BraveDevToolsUIBindings&) = delete;

 private:
  FRIEND_TEST_ALL_PREFIXES(BraveDevToolsUIBindingsBrowserTest, ThemeTest);

  // DevToolsUIBindings overrides:
  void GetPreferences(DispatchCallback callback) override;
};

#endif  // BRAVE_BROWSER_DEVTOOLS_BRAVE_DEVTOOLS_UI_BINDINGS_H_
