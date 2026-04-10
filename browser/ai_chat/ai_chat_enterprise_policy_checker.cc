// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/ai_chat_enterprise_policy_checker.h"

#include "base/no_destructor.h"

namespace ai_chat {

AIChatEnterprisePolicyChecker::AIChatEnterprisePolicyChecker(
    actor::EnterprisePolicyChecker::UrlBlockReason reason)
    : reason_(reason) {}
AIChatEnterprisePolicyChecker::~AIChatEnterprisePolicyChecker() = default;

actor::EnterprisePolicyChecker::UrlBlockReason
AIChatEnterprisePolicyChecker::Evaluate(const GURL& url) const {
  return reason_;
}

const actor::EnterprisePolicyChecker*
AIChatEnterprisePolicyChecker::NoEnterprisePolicyChecker() {
  static base::NoDestructor<AIChatEnterprisePolicyChecker> checker(
      actor::EnterprisePolicyChecker::UrlBlockReason::kNotBlocked);
  return checker.get();
}

void AIChatEnterprisePolicyChecker::ValidateContentSentToRenderer(
    content::RenderFrameHost* frame,
    const std::string& content,
    actor::EnterprisePolicyChecker::ContentValidationCallback callback) const {
  std::move(callback).Run(
      actor::EnterprisePolicyChecker::ContentValidationReason::kAllowed);
}

}  // namespace ai_chat
