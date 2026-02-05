/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_WELCOME_PAGE_WELCOME_DOM_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_WELCOME_PAGE_WELCOME_DOM_HANDLER_H_

#include <optional>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "brave/browser/ui/webui/brave_education/brave_education_server_checker.h"
#include "chrome/browser/shell_integration.h"
#include "content/public/browser/web_ui_message_handler.h"

class Profile;
class Browser;

// The handler for Javascript messages for the chrome://welcome page
class WelcomeDOMHandler : public content::WebUIMessageHandler {
 public:
  explicit WelcomeDOMHandler(Profile* profile);
  WelcomeDOMHandler(const WelcomeDOMHandler&) = delete;
  WelcomeDOMHandler& operator=(const WelcomeDOMHandler&) = delete;
  ~WelcomeDOMHandler() override;

  // WebUIMessageHandler implementation.
  void RegisterMessages() override;

 private:
  void HandleImportNowRequested(const base::ListValue& args);
  void HandleRecordP3A(const base::ListValue& args);
  void HandleGetDefaultBrowser(const base::ListValue& args);
  void SetLocalStateBooleanEnabled(const std::string& path,
                                   const base::ListValue& args);
  void OnGetDefaultBrowser(shell_integration::DefaultWebClientState state,
                           const std::u16string& name);
  void SetP3AEnabled(const base::ListValue& args);
  void HandleOpenSettingsPage(const base::ListValue& args);
  void HandleSetMetricsReportingEnabled(const base::ListValue& args);
  void HandleEnableWebDiscovery(const base::ListValue& args);
  void HandleGetWelcomeCompleteURL(const base::ListValue& args);

  void OnGettingStartedServerCheck(const std::string& callback_id,
                                   bool available);

  Browser* GetBrowser();

  size_t last_onboarding_phase_ = 0;
  std::u16string default_browser_name_;
  raw_ptr<Profile> profile_ = nullptr;
  brave_education::BraveEducationServerChecker brave_education_server_checker_;
  base::WeakPtrFactory<WelcomeDOMHandler> weak_ptr_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_WEBUI_WELCOME_PAGE_WELCOME_DOM_HANDLER_H_
