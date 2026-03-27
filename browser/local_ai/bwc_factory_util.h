// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_LOCAL_AI_BWC_FACTORY_UTIL_H_
#define BRAVE_BROWSER_LOCAL_AI_BWC_FACTORY_UTIL_H_

#include <string_view>

#include "brave/components/local_ai/core/local_ai_service_base.h"

namespace content {
class BrowserContext;
}  // namespace content

namespace local_ai {

// Creates a BackgroundWebContentsFactory that constructs a
// BackgroundWebContentsImpl navigating to |url| and tagged
// with |task_manager_title_id| in the task manager.
LocalAIServiceBase::BackgroundWebContentsFactory MakeBWCFactory(
    content::BrowserContext* context,
    std::string_view url,
    int task_manager_title_id);

}  // namespace local_ai

#endif  // BRAVE_BROWSER_LOCAL_AI_BWC_FACTORY_UTIL_H_
