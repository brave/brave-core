/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_EMAIL_ALIASES_EMAIL_ALIASES_CONTROLLER_H_
#define BRAVE_BROWSER_UI_EMAIL_ALIASES_EMAIL_ALIASES_CONTROLLER_H_

#include "base/memory/raw_ptr.h"
#include "base/scoped_observation.h"
#include "brave/components/email_aliases/email_aliases_service.h"

class BrowserView;
class WebUIBubbleManager;

namespace content {
struct ContextMenuParams;
}

namespace base {
template <>
struct ScopedObservationTraits<email_aliases::EmailAliasesService,
                               email_aliases::EmailAliasesBubbleObserver> {
  static void AddObserver(email_aliases::EmailAliasesService* source,
                          email_aliases::EmailAliasesBubbleObserver* observer) {
    source->AddBubbleObserver(observer);
  }
  static void RemoveObserver(
      email_aliases::EmailAliasesService* source,
      email_aliases::EmailAliasesBubbleObserver* observer) {
    source->RemoveBubbleObserver(observer);
  }
};
}  // namespace base

namespace email_aliases {

class EmailAliasesController : public EmailAliasesBubbleObserver {
 public:
  EmailAliasesController(BrowserView* browser_view,
                         EmailAliasesService* email_aliases_service);
  ~EmailAliasesController() override;
  EmailAliasesController(const EmailAliasesController&) = delete;
  EmailAliasesController& operator=(const EmailAliasesController&) = delete;

  bool IsAvailableFor(const content::ContextMenuParams& params) const;

  void ShowBubble(uint64_t field_renderer_id);
  void CloseBubble();
  void OpenSettingsPage();

 private:
  // EmailAliasesBubbleObserver:
  void OnAliasCreationComplete(
      const std::optional<std::string>& email) override;
  void OnInvokeManageAliases() override;

  raw_ptr<BrowserView> browser_view_ = nullptr;
  raw_ptr<EmailAliasesService> email_aliases_service_ = nullptr;

  std::unique_ptr<WebUIBubbleManager> bubble_;
  base::ScopedObservation<EmailAliasesService, EmailAliasesBubbleObserver>
      observation_{this};
};

}  // namespace email_aliases

#endif  // BRAVE_BROWSER_UI_EMAIL_ALIASES_EMAIL_ALIASES_CONTROLLER_H_
