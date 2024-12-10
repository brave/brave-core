/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/content_settings/renderer/brave_content_settings_agent_impl.h"
#include "components/dom_distiller/content/renderer/distillability_agent.h"

// Prevents unnecessary js console logs spam.
#define DistillabilityAgent(render_frame, dcheck_is_on) \
  DistillabilityAgent(render_frame, false)

#include "src/chrome/renderer/chrome_content_renderer_client.cc"

#undef DistillabilityAgent
