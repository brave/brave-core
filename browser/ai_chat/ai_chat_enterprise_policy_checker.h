// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_AI_CHAT_AI_CHAT_ENTERPRISE_POLICY_CHECKER_H_
#define BRAVE_BROWSER_AI_CHAT_AI_CHAT_ENTERPRISE_POLICY_CHECKER_H_

#include "chrome/browser/actor/enterprise_policy_url_checker.h"

class GURL;

namespace ai_chat {

class AIChatEnterprisePolicyChecker : public actor::EnterprisePolicyUrlChecker {
 public:
  static const actor::EnterprisePolicyUrlChecker* NoEnterprisePolicyChecker();

  explicit AIChatEnterprisePolicyChecker(
      actor::EnterprisePolicyBlockReason reason);
  ~AIChatEnterprisePolicyChecker();

  actor::EnterprisePolicyBlockReason Evaluate(const GURL& url) const override;

 private:
  actor::EnterprisePolicyBlockReason reason_;
};

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_AI_CHAT_AI_CHAT_ENTERPRISE_POLICY_CHECKER_H_
