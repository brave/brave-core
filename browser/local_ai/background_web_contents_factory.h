/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_LOCAL_AI_BACKGROUND_WEB_CONTENTS_FACTORY_H_
#define BRAVE_BROWSER_LOCAL_AI_BACKGROUND_WEB_CONTENTS_FACTORY_H_

#include <memory>
#include <optional>

#include "base/functional/callback.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/local_ai/core/background_web_contents.h"
#include "services/network/public/mojom/web_sandbox_flags.mojom-shared.h"

class GURL;
class Profile;

namespace local_ai {

// Reply for CreateBackgroundWebContents(). Hands back the created worker
// environment and the guest OTR profile it lives in (both null on failure).
// The caller decides what to do with the profile (e.g. observe it for
// destruction); the factory does not observe it.
using BackgroundWebContentsCreatedCallback =
    base::OnceCallback<void(std::unique_ptr<BackgroundWebContents>,
                            Profile* otr_profile)>;

// Creates the guest profile's primary OTR profile asynchronously, then builds
// a BackgroundWebContentsImpl in it, navigated to `url` and tagged in the task
// manager with `task_manager_title_id`. When `sandbox_flags` is nullopt the
// worker uses the default background sandbox; pass a value to override (e.g.
// kNone for a cross-origin-isolated worker that must be fully unsandboxed).
//
// `delegate` is held weakly: a caller destroyed during the async guest-profile
// creation safely yields (nullptr, nullptr) instead of a use-after-free. This
// covers both a NoDestructor singleton caller (whose weak ptr never
// invalidates) and a caller owned by another object (whose weak ptr drops).
void CreateBackgroundWebContents(
    const GURL& url,
    int task_manager_title_id,
    std::optional<network::mojom::WebSandboxFlags> sandbox_flags,
    base::WeakPtr<BackgroundWebContents::Delegate> delegate,
    BackgroundWebContentsCreatedCallback created);

}  // namespace local_ai

#endif  // BRAVE_BROWSER_LOCAL_AI_BACKGROUND_WEB_CONTENTS_FACTORY_H_
