/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_EDUCATION_BRAVE_EDUCATION_PAGE_DELEGATE_DESKTOP_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_EDUCATION_BRAVE_EDUCATION_PAGE_DELEGATE_DESKTOP_H_

#include "base/memory/raw_ref.h"
#include "brave/browser/ui/webui/brave_browser_command/brave_browser_command_handler.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "ui/base/window_open_disposition.h"
#include "url/gurl.h"

class BrowserWindowInterface;

namespace brave_education {

// Handles browser-level requests from the Brave Education WebUI.
class BraveEducationPageDelegateDesktop
    : public BraveBrowserCommandHandler::Delegate {
 public:
  explicit BraveEducationPageDelegateDesktop(
      BrowserWindowInterface& window_interface);
  ~BraveEducationPageDelegateDesktop() override;

  void OpenURL(const GURL& url, WindowOpenDisposition disposition) override;
  void OpenRewardsPanel() override;
  void OpenVPNPanel() override;
#if BUILDFLAG(ENABLE_AI_CHAT)
  void OpenAIChat() override;
#endif

 private:
  // The browser window interface for tab w/ brave_education's WebUI.
  raw_ref<BrowserWindowInterface> window_interface_;
};

}  // namespace brave_education

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_EDUCATION_BRAVE_EDUCATION_PAGE_DELEGATE_DESKTOP_H_
