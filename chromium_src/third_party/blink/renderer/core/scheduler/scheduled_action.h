/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_SCHEDULER_SCHEDULED_ACTION_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_SCHEDULER_SCHEDULED_ACTION_H_

#include "brave/components/brave_page_graph/common/buildflags.h"

namespace blink {
class ScriptFetchOptions;
}

#define arguments_                                                  \
  arguments_;                                                       \
  IF_BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH, int parent_script_id_ = 0;) \
  ScriptFetchOptions GetScriptFetchOptions() const

#include "src/third_party/blink/renderer/core/scheduler/scheduled_action.h"  // IWYU pragma: export

#undef arguments_

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_SCHEDULER_SCHEDULED_ACTION_H_
