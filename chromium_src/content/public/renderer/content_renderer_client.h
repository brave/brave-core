/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CONTENT_PUBLIC_RENDERER_CONTENT_RENDERER_CLIENT_H_
#define BRAVE_CHROMIUM_SRC_CONTENT_PUBLIC_RENDERER_CONTENT_RENDERER_CLIENT_H_

#include <content/public/renderer/content_renderer_client.h>

namespace base {
class CommandLine;
}

namespace content {

// Runs the feature initialization sequence used by RenderThreadImpl, without
// starting a renderer process. Exported for tests in component builds.
CONTENT_EXPORT void InitializeRuntimeFeaturesForTesting(
    ContentRendererClient& renderer_client,
    const base::CommandLine& command_line);

}  // namespace content

#endif  // BRAVE_CHROMIUM_SRC_CONTENT_PUBLIC_RENDERER_CONTENT_RENDERER_CLIENT_H_
