// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_BROWSER_STATE_BRAVE_BROWSER_STATE_MANAGER_IMPL_H_
#define BRAVE_IOS_BROWSER_BROWSER_STATE_BRAVE_BROWSER_STATE_MANAGER_IMPL_H_

#include "ios/chrome/browser/browser_state/chrome_browser_state_manager_impl.h"

class ChromeBrowserState;

/// This extends the behaviors of the ChromeBrowserStateManagerImpl
class BraveBrowserStateManagerImpl : public ChromeBrowserStateManagerImpl {
 protected:
  void DoFinalInitForServices(ChromeBrowserState* browser_state) override;
};

#endif  // BRAVE_IOS_BROWSER_BROWSER_STATE_BRAVE_BROWSER_STATE_MANAGER_IMPL_H_
