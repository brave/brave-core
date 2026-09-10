/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <content/renderer/render_thread_impl.cc>

namespace content {

void InitializeRuntimeFeaturesForTesting(
    ContentRendererClient& renderer_client,
    const base::CommandLine& command_line) {
  // Keep the order used by RenderThreadImpl::InitializeWebKit().
  renderer_client.SetRuntimeFeaturesDefaultsBeforeBlinkInitialization();
  SetRuntimeFeaturesDefaultsAndUpdateFromArgs(command_line);
}

}  // namespace content
