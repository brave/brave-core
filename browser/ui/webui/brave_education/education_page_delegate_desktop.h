/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_EDUCATION_EDUCATION_PAGE_DELEGATE_DESKTOP_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_EDUCATION_EDUCATION_PAGE_DELEGATE_DESKTOP_H_

#include "base/memory/raw_ref.h"
#include "brave/browser/ui/webui/brave_browser_command/brave_browser_command_handler.h"
#include "ui/base/window_open_disposition.h"
#include "url/gurl.h"

namespace tabs {
class TabInterface;
}

namespace brave_education {

// Handles browser-level requests from the Brave Education WebUI.
class EducationPageDelegateDesktop
    : public BraveBrowserCommandHandler::Delegate {
 public:
  explicit EducationPageDelegateDesktop(tabs::TabInterface& tab);
  ~EducationPageDelegateDesktop() override;

  void OpenURL(const GURL& url, WindowOpenDisposition disposition) override;
  void OpenRewardsPanel() override;
  void OpenVPNPanel() override;
  void OpenAIChat() override;

 private:
  // The tab that contains the Brave Education WebUI.
  raw_ref<tabs::TabInterface> tab_;
};

}  // namespace brave_education

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_EDUCATION_EDUCATION_PAGE_DELEGATE_DESKTOP_H_
