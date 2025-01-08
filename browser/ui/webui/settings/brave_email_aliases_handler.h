// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_EMAIL_ALIASES_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_EMAIL_ALIASES_HANDLER_H_

#include <memory>
#include <optional>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/ui/webui/settings/settings_page_ui_handler.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/webui_config.h"
#include "content/public/common/url_constants.h"
#include "services/network/public/cpp/simple_url_loader.h"

class GURL;
class Profile;

class BraveEmailAliasesHandler : public settings::SettingsPageUIHandler {
 public:
  BraveEmailAliasesHandler();
  BraveEmailAliasesHandler(const BraveEmailAliasesHandler&) = delete;
  BraveEmailAliasesHandler& operator=(const BraveEmailAliasesHandler&) = delete;
  ~BraveEmailAliasesHandler() override;

  void GenerateAlias(const base::Value::List& args);
  void OnGenerateAliasResponse(const std::string& callback_id,
                               std::optional<std::string> response_body);
  void GetAliases(const base::Value::List& args);
  void OnGetAliasesResponse(const std::string& callback_id,
                            std::optional<std::string> response_body);
  void CreateAlias(const base::Value::List& args);
  void OnCreateAliasResponse(const std::string& callback_id,
                             const std::string& alias_email,
                             const std::string& note,
                             std::optional<std::string> response_body);
  void UpdateAlias(const base::Value::List& args);
  void OnUpdateAliasResponse(const std::string& callback_id,
                             const std::string& alias_email,
                             const std::string& note,
                             std::optional<std::string> response_body);
  void DeleteAlias(const base::Value::List& args);
  void OnDeleteAliasResponse(const std::string& callback_id,
                             const std::string& alias_email,
                             std::optional<std::string> response_body);
  void RequestAccount(const base::Value::List& args);
  void OnRequestAccountResponse(const std::string& callback_id,
                                const std::string& account_email,
                                std::optional<std::string> response_body);
  void GetSession(const base::Value::List& args);
  void OnGetSessionResponse(const std::string& callback_id,
                            std::optional<std::string> response_body);
  void GetAccountEmail(const base::Value::List& args);
  void Logout(const base::Value::List& args);
  void CloseBubble(const base::Value::List& args);
  void FillField(const base::Value::List& args);

 private:
  // SettingsPageUIHandler overrides
  void RegisterMessages() override;
  void OnJavascriptAllowed() override {}
  void OnJavascriptDisallowed() override {}

  Profile* GetProfile();
  std::string GetStringPref(const std::string& pref_name);
  void SetStringPref(const std::string& pref_name, const std::string& value);
  void ClearPref(const std::string& pref_name);

  void ApiFetch(const GURL& url,
                const char* method,
                const std::optional<std::string>& bearer_token,
                const base::Value::Dict& bodyValue,
                network::SimpleURLLoader::BodyAsStringCallback
                    download_to_string_callback);

  void SetNote(const std::string& alias_email, const std::string& note);
  void DeleteNote(const std::string& alias_email);
  std::optional<std::string> GetNote(const std::string& alias_email);

  raw_ptr<Profile> profile_ = nullptr;
  std::string verification_token_;
  std::string session_token_;
  std::unique_ptr<network::SimpleURLLoader> simple_url_loader_;

  base::WeakPtrFactory<BraveEmailAliasesHandler> weak_factory_{this};
};

/*
class BraveEmailAliasesHandlerConfig
    : public content::DefaultWebUIConfig<BraveEmailAliasesHandler> {
 public:
  BraveEmailAliasesHandlerConfig()
      : DefaultWebUIConfig(content::kChromeUIScheme, kEmailAliasesBubbleHost) {}
};
*/
#endif  // BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_EMAIL_ALIASES_HANDLER_H_
