/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/core/exported/web_dev_tools_agent_impl.h"

#include "brave/third_party/blink/renderer/core/inspector/inspector_brave_agent.h"
#include "third_party/blink/renderer/core/inspector/devtools_session.h"

namespace blink {

void WebDevToolsAgentImpl::AttachSession(DevToolsSession* session,
                                         bool restore) {
  AttachSession_ChromiumImpl(session, restore);
  session->CreateAndAppend<InspectorBraveAgent>();
}

}  // namespace blink

#define AttachSession AttachSession_ChromiumImpl

#include "src/third_party/blink/renderer/core/exported/web_dev_tools_agent_impl.cc"  // IWYU pragma: export

#undef AttachSession
