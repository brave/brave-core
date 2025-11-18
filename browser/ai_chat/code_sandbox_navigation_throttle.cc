// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/code_sandbox_navigation_throttle.h"

#include "base/strings/string_util.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/navigation_throttle.h"
#include "content/public/browser/web_contents.h"
#include "url/gurl.h"

namespace ai_chat {

// static
void CodeSandboxNavigationThrottle::MaybeCreateAndAdd(
    content::NavigationThrottleRegistry& registry) {
  auto* context =
      registry.GetNavigationHandle().GetWebContents()->GetBrowserContext();
  auto* profile = Profile::FromBrowserContext(context);
  if (!profile->IsOffTheRecord() ||
      !profile->GetOTRProfileID().IsCodeSandbox()) {
    return;
  }

  registry.AddThrottle(
      std::make_unique<CodeSandboxNavigationThrottle>(registry));
}

CodeSandboxNavigationThrottle::CodeSandboxNavigationThrottle(
    content::NavigationThrottleRegistry& registry)
    : NavigationThrottle(registry) {}

CodeSandboxNavigationThrottle::~CodeSandboxNavigationThrottle() = default;

content::NavigationThrottle::ThrottleCheckResult
CodeSandboxNavigationThrottle::WillStartRequest() {
  auto spec = navigation_handle()->GetURL().spec();
  if (base::StartsWith(spec, kAIChatCodeSandboxUIURL)) {
    return content::NavigationThrottle::PROCEED;
  }

  return content::NavigationThrottle::CANCEL;
}

content::NavigationThrottle::ThrottleCheckResult
CodeSandboxNavigationThrottle::WillRedirectRequest() {
  return WillStartRequest();
}

const char* CodeSandboxNavigationThrottle::GetNameForLogging() {
  return "CodeSandboxNavigationThrottle";
}

}  // namespace ai_chat

