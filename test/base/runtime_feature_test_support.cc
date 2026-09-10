/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/test/base/runtime_feature_test_support.h"

#include "base/command_line.h"
#include "base/functional/callback_helpers.h"
#include "base/test/scoped_command_line.h"
#include "base/test/task_environment.h"
#include "brave/renderer/brave_content_renderer_client.h"
#include "content/public/renderer/content_renderer_client.h"

namespace brave {

std::vector<blink::WebRuntimeFeatures::RuntimeFeatureStateForTesting>
GetRendererRuntimeFeatureStatesForTesting() {
  base::test::ScopedCommandLine command_line;
  *command_line.GetProcessCommandLine() =
      base::CommandLine(base::CommandLine::NO_PROGRAM);
  base::test::TaskEnvironment task_environment;
  base::ScopedClosureRunner restore_runtime_features(
      blink::WebRuntimeFeatures::BackupAndResetRuntimeFeaturesForTesting());

  BraveContentRendererClient renderer_client;
  content::InitializeRuntimeFeaturesForTesting(
      renderer_client, *command_line.GetProcessCommandLine());
  return blink::WebRuntimeFeatures::GetRuntimeFeatureStatesForTesting();
}

}  // namespace brave
