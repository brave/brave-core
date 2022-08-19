/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_INSPECTOR_INSPECTOR_PAGE_AGENT_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_INSPECTOR_INSPECTOR_PAGE_AGENT_H_

#include "third_party/blink/renderer/core/inspector/protocol/page.h"

#define clearCompilationCache                                                  \
  NotUsed();                                                                   \
  protocol::Response generatePageGraph(String* data) override;                 \
  protocol::Response generatePageGraphNodeReport(                              \
      int node_id, std::unique_ptr<protocol::Array<String>>* report) override; \
  protocol::Response clearCompilationCache

#include "src/third_party/blink/renderer/core/inspector/inspector_page_agent.h"

#undef clearCompilationCache

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_INSPECTOR_INSPECTOR_PAGE_AGENT_H_
