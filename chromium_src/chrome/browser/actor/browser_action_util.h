// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_ACTOR_BROWSER_ACTION_UTIL_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_ACTOR_BROWSER_ACTION_UTIL_H_

#include <chrome/browser/actor/browser_action_util.h>  // IWYU pragma: export

namespace actor {

// This exports CreateToolRequest so it can be used to verify the navigation
// tool for the ai chat module.
std::unique_ptr<ToolRequest> CreateToolRequestForTesting(
    const optimization_guide::proto::Action& action);

}  // namespace actor

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_ACTOR_BROWSER_ACTION_UTIL_H_
