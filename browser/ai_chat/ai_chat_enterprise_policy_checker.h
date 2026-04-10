// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_AI_CHAT_AI_CHAT_ENTERPRISE_POLICY_CHECKER_H_
#define BRAVE_BROWSER_AI_CHAT_AI_CHAT_ENTERPRISE_POLICY_CHECKER_H_

#include "chrome/browser/actor/enterprise_policy_checker.h"

class GURL;

namespace ai_chat {

class AIChatEnterprisePolicyChecker : public actor::EnterprisePolicyChecker {
 public:
  static const actor::EnterprisePolicyChecker* NoEnterprisePolicyChecker();

  explicit AIChatEnterprisePolicyChecker(
      actor::EnterprisePolicyChecker::UrlBlockReason reason);
  ~AIChatEnterprisePolicyChecker() override;

  actor::EnterprisePolicyChecker::UrlBlockReason Evaluate(
      const GURL& url) const override;

  void ValidateContentSentToRenderer(
      content::RenderFrameHost* frame,
      const std::string& content,
      actor::EnterprisePolicyChecker::ContentValidationCallback callback)
      const override;

 private:
  actor::EnterprisePolicyChecker::UrlBlockReason reason_;
};

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_AI_CHAT_AI_CHAT_ENTERPRISE_POLICY_CHECKER_H_
