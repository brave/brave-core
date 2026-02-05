// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/ai_chat_enterprise_policy_checker.h"

#include "base/no_destructor.h"

namespace ai_chat {

AIChatEnterprisePolicyChecker::AIChatEnterprisePolicyChecker(
    actor::EnterprisePolicyBlockReason reason)
    : reason_(reason) {}
AIChatEnterprisePolicyChecker::~AIChatEnterprisePolicyChecker() = default;

bool AIChatEnterprisePolicyChecker::CanActOnWeb() const {
  return true;
}

actor::EnterprisePolicyChecker::CannotActReason
AIChatEnterprisePolicyChecker::CannotActOnWebReason() const {
  return actor::EnterprisePolicyChecker::CannotActReason::kNone;
}

actor::EnterprisePolicyBlockReason AIChatEnterprisePolicyChecker::Evaluate(
    const GURL& url) const {
  return reason_;
}

const actor::EnterprisePolicyChecker*
AIChatEnterprisePolicyChecker::NoEnterprisePolicyChecker() {
  static base::NoDestructor<AIChatEnterprisePolicyChecker> checker(
      actor::EnterprisePolicyBlockReason::kNotBlocked);
  return checker.get();
}

}  // namespace ai_chat
