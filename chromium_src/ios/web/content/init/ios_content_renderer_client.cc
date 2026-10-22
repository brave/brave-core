// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ios/web/content/init/ios_content_renderer_client.h"

#include "brave/ios/web/content/shields_settings_agent.h"
#include "components/js_injection/renderer/js_communication.h"

// Both self-delete with their RenderFrame.
#define JsCommunication(render_frame) \
  JsCommunication(render_frame);      \
  new brave_shields::ShieldsSettingsAgent(render_frame)

#include <ios/web/content/init/ios_content_renderer_client.cc>

#undef JsCommunication
