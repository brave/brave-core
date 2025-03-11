// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/settings/brave_email_aliases_handler.h"

#include <memory>
#include <string>
#include <utility>

#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/values.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/browser/ui/webui/email_aliases/email_aliases_bubble_ui.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/email_aliases/browser/pref_names.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/singleton_tabs.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "content/public/browser/web_ui.h"
#include "net/http/http_request_headers.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "url/gurl.h"

constexpr int kMaxResponseLength = 32768;

const char kAccountsServiceRequestURL[] =
    "https://accounts.bsg.bravesoftware.com/v2/verify/init";
const char kAccountsServiceVerifyURL[] =
    "https://accounts.bsg.bravesoftware.com/v2/verify/result";
const char kMappingServiceManageURL[] =
    "https://aliases.bravesoftware.com/manage";
const char kMappingServiceGenerateURL[] =
    "https://aliases.bravesoftware.com/generate";
const char kBraveApiKey[] = "px6zQ7rIMGaS8FE6cmpUp45WQTFJYXgo7ZlBhrFK";

const net::NetworkTrafficAnnotationTag traffic_annotation =
    net::DefineNetworkTrafficAnnotation("email_aliases_mapping_service", R"(
    semantics {
      sender: "Email Aliases service"
      description:
        "Call Email Aliases Mapping Service API"
      trigger:
        "When the user connects to the Email Mapping Service, to "
        "Generate, Create, Read, Update, or Delete Email Aliases. "
      destination: BRAVE_OWNED_SERVICE
    }
    policy {
      cookies_allowed: YES
    })");

BraveEmailAliasesHandler::BraveEmailAliasesHandler() = default;

BraveEmailAliasesHandler::~BraveEmailAliasesHandler() = default;

Profile* BraveEmailAliasesHandler::GetProfile() {
  if (!profile_) {
    profile_ = Profile::FromWebUI(web_ui());
  }
  return profile_;
}

std::string BraveEmailAliasesHandler::GetStringPref(
    const std::string& pref_name) {
  return GetProfile()->GetPrefs()->GetString(pref_name);
}

void BraveEmailAliasesHandler::SetStringPref(const std::string& pref_name,
                                             const std::string& value) {
  GetProfile()->GetPrefs()->SetString(pref_name, value);
}

void BraveEmailAliasesHandler::ClearPref(const std::string& pref_name) {
  GetProfile()->GetPrefs()->ClearPref(pref_name);
}

void BraveEmailAliasesHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "email_aliases.generateAlias",
      base::BindRepeating(&BraveEmailAliasesHandler::GenerateAlias,
                          base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "email_aliases.getAliases",
      base::BindRepeating(&BraveEmailAliasesHandler::GetAliases,
                          base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "email_aliases.createAlias",
      base::BindRepeating(&BraveEmailAliasesHandler::CreateAlias,
                          base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "email_aliases.updateAlias",
      base::BindRepeating(&BraveEmailAliasesHandler::UpdateAlias,
                          base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "email_aliases.deleteAlias",
      base::BindRepeating(&BraveEmailAliasesHandler::DeleteAlias,
                          base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "email_aliases.requestAccount",
      base::BindRepeating(&BraveEmailAliasesHandler::RequestAccount,
                          base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "email_aliases.getSession",
      base::BindRepeating(&BraveEmailAliasesHandler::GetSession,
                          base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "email_aliases.getAccountEmail",
      base::BindRepeating(&BraveEmailAliasesHandler::GetAccountEmail,
                          base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "email_aliases.logout",
      base::BindRepeating(&BraveEmailAliasesHandler::Logout,
                          base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "email_aliases.closeBubble",
      base::BindRepeating(&BraveEmailAliasesHandler::CloseBubble,
                          base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "email_aliases.fillField",
      base::BindRepeating(&BraveEmailAliasesHandler::FillField,
                          base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "email_aliases.showSettingsPage",
      base::BindRepeating(&BraveEmailAliasesHandler::ShowSettingsPage,
                          base::Unretained(this)));
}

void BraveEmailAliasesHandler::SetNote(const std::string& alias_email,
                                       const std::string& note) {
  ScopedDictPrefUpdate update(GetProfile()->GetPrefs(), kEmailAliasesNotes);
  update->Set(alias_email, note);
}

void BraveEmailAliasesHandler::DeleteNote(const std::string& alias_email) {
  ScopedDictPrefUpdate update(GetProfile()->GetPrefs(), kEmailAliasesNotes);
  update->Remove(alias_email);
}

std::optional<std::string> BraveEmailAliasesHandler::GetNote(
    const std::string& alias_email) {
  const auto* note = GetProfile()
                         ->GetPrefs()
                         ->GetDict(kEmailAliasesNotes)
                         .FindString(alias_email);
  return note ? std::optional<std::string>(*note) : std::nullopt;
}

void BraveEmailAliasesHandler::ApiFetch(
    const GURL& url,
    const char* method,
    const std::optional<std::string>& bearer_token,
    const base::Value::Dict& bodyValue,
    network::SimpleURLLoader::BodyAsStringCallback
        download_to_string_callback) {
  auto resource_request = std::make_unique<network::ResourceRequest>();
  resource_request->url = url;
  resource_request->method = method;
  if (bearer_token) {
    resource_request->headers.SetHeader(
        "Authorization", std::string("Bearer ") + bearer_token.value());
  }
  resource_request->headers.SetHeader("X-API-key", kBraveApiKey);
  simple_url_loader_ = network::SimpleURLLoader::Create(
      std::move(resource_request), traffic_annotation);
  if (!bodyValue.empty() && method != net::HttpRequestHeaders::kGetMethod &&
      method != net::HttpRequestHeaders::kHeadMethod) {
    auto body = base::WriteJson(bodyValue);
    CHECK(body);
    simple_url_loader_->AttachStringForUpload(body.value(), "application/json");
  }
  simple_url_loader_->DownloadToString(
      GetProfile()->GetURLLoaderFactory().get(),
      std::move(download_to_string_callback), kMaxResponseLength);
}

void BraveEmailAliasesHandler::GenerateAlias(const base::Value::List& args) {
  AllowJavascript();
  CHECK_EQ(1U, args.size());
  const auto callback_id = args[0].GetString();
  ApiFetch(GURL(kMappingServiceGenerateURL),
           net::HttpRequestHeaders::kGetMethod,
           GetStringPref(kEmailAliasesAuthToken), base::Value::Dict(),
           base::BindOnce(&BraveEmailAliasesHandler::OnGenerateAliasResponse,
                          weak_factory_.GetWeakPtr(), callback_id));
}

void BraveEmailAliasesHandler::OnGenerateAliasResponse(
    const std::string& callback_id,
    std::optional<std::string> response_body) {
  if (response_body) {
    ResolveJavascriptCallback(base::Value(callback_id), response_body.value());
  } else {
    RejectJavascriptCallback(base::Value(callback_id),
                             base::Value("alias generation failed"));
  }
}

void BraveEmailAliasesHandler::GetAliases(const base::Value::List& args) {
  AllowJavascript();
  CHECK_EQ(1U, args.size());
  const auto callback_id = args[0].GetString();
  ApiFetch(GURL(kMappingServiceManageURL + std::string("?status=active")),
           net::HttpRequestHeaders::kGetMethod,
           GetStringPref(kEmailAliasesAuthToken), base::Value::Dict(),
           base::BindOnce(&BraveEmailAliasesHandler::OnGetAliasesResponse,
                          weak_factory_.GetWeakPtr(), callback_id));
}

void BraveEmailAliasesHandler::OnGetAliasesResponse(
    const std::string& callback_id,
    std::optional<std::string> response_body) {
  AllowJavascript();
  if (!response_body) {
    RejectJavascriptCallback(base::Value(callback_id),
                             base::Value("no response body"));
    return;
  }
  std::optional<base::Value> response_value =
      base::JSONReader::Read(response_body.value());
  if (response_value && response_value.value().is_list()) {
    auto alias_list = base::Value::List();
    for (auto& item : response_value.value().GetList()) {
      if (item.is_dict()) {
        auto& item_dict = item.GetDict();
        auto* email_value_ptr = item_dict.FindString("alias");
        auto* status_value_ptr = item_dict.FindString("status");
        auto note_value =
            email_value_ptr ? GetNote(*email_value_ptr) : std::nullopt;
        alias_list.Append(
            base::Value::Dict()
                .Set("email", email_value_ptr ? *email_value_ptr : "")
                .Set("note", note_value.value_or(""))
                .Set("status", status_value_ptr ? *status_value_ptr : ""));
      }
    }
    ResolveJavascriptCallback(base::Value(callback_id), alias_list);
  } else {
    RejectJavascriptCallback(base::Value(callback_id),
                             base::Value("bad response"));
  }
}

void BraveEmailAliasesHandler::CreateAlias(const base::Value::List& args) {
  AllowJavascript();
  CHECK_EQ(3U, args.size());
  const auto callback_id = args[0].GetString();
  const auto alias_email = args[1].GetString();
  const auto note = args[2].GetString();
  auto bodyValue = base::Value::Dict().Set("alias", alias_email);
  ApiFetch(GURL(kMappingServiceManageURL), net::HttpRequestHeaders::kPostMethod,
           GetStringPref(kEmailAliasesAuthToken), bodyValue,
           base::BindOnce(&BraveEmailAliasesHandler::OnCreateAliasResponse,
                          weak_factory_.GetWeakPtr(), callback_id, alias_email,
                          note));
}

void BraveEmailAliasesHandler::OnCreateAliasResponse(
    const std::string& callback_id,
    const std::string& alias_email,
    const std::string& note,
    std::optional<std::string> response_body) {
  SetNote(alias_email, note);
  ResolveJavascriptCallback(base::Value(callback_id), base::Value());
}

void BraveEmailAliasesHandler::DeleteAlias(const base::Value::List& args) {
  AllowJavascript();
  CHECK_EQ(2U, args.size());
  const auto callback_id = args[0].GetString();
  const auto alias_email = args[1].GetString();
  auto bodyValue = base::Value::Dict().Set("alias", alias_email);
  ApiFetch(
      GURL(kMappingServiceManageURL), net::HttpRequestHeaders::kDeleteMethod,
      GetStringPref(kEmailAliasesAuthToken), bodyValue,
      base::BindOnce(&BraveEmailAliasesHandler::OnDeleteAliasResponse,
                     weak_factory_.GetWeakPtr(), callback_id, alias_email));
}

void BraveEmailAliasesHandler::OnDeleteAliasResponse(
    const std::string& callback_id,
    const std::string& alias_email,
    std::optional<std::string> response_body) {
  DeleteNote(alias_email);
  ResolveJavascriptCallback(base::Value(callback_id), base::Value());
}

void BraveEmailAliasesHandler::UpdateAlias(const base::Value::List& args) {
  AllowJavascript();
  CHECK_EQ(4U, args.size());
  const auto callback_id = args[0].GetString();
  if (!args[1].is_string() || !args[2].is_string() || !args[3].is_bool()) {
    RejectJavascriptCallback(base::Value(callback_id),
                             base::Value("unexpected values"));
    return;
  }
  const auto alias_email = args[1].GetString();
  const auto note = args[2].GetString();
  const std::string status = args[3].GetBool() ? "active" : "paused";
  auto bodyValue =
      base::Value::Dict().Set("alias", alias_email).Set("status", status);
  ApiFetch(GURL(kMappingServiceManageURL), net::HttpRequestHeaders::kPutMethod,
           GetStringPref(kEmailAliasesAuthToken), bodyValue,
           base::BindOnce(&BraveEmailAliasesHandler::OnUpdateAliasResponse,
                          weak_factory_.GetWeakPtr(), callback_id, alias_email,
                          note));
}

void BraveEmailAliasesHandler::OnUpdateAliasResponse(
    const std::string& callback_id,
    const std::string& alias_email,
    const std::string& note,
    std::optional<std::string> response_body) {
  SetNote(alias_email, note);
  ResolveJavascriptCallback(base::Value(callback_id), base::Value());
}

void BraveEmailAliasesHandler::GetSession(const base::Value::List& args) {
  AllowJavascript();
  CHECK_EQ(1U, args.size());
  const auto callback_id = args[0].GetString();
  auto bodyValue = base::Value::Dict().Set("wait", true);
  ApiFetch(GURL(kAccountsServiceVerifyURL),
           net::HttpRequestHeaders::kPostMethod,
           GetStringPref(kEmailAliasesVerificationToken), bodyValue,
           base::BindOnce(&BraveEmailAliasesHandler::OnGetSessionResponse,
                          weak_factory_.GetWeakPtr(), callback_id));
}

void BraveEmailAliasesHandler::OnGetSessionResponse(
    const std::string& callback_id,
    std::optional<std::string> response_body) {
  AllowJavascript();
  if (response_body) {
    std::optional<base::Value> response_value =
        base::JSONReader::Read(response_body.value());
    if (response_value && response_value->is_dict()) {
      const auto* session_token = response_value->GetDict().Find("authToken");
      if (session_token && session_token->is_string()) {
        // Store the session token for long-term use.
        SetStringPref(kEmailAliasesAuthToken, session_token->GetString());
        // Acknowledge success to the caller.
        ResolveJavascriptCallback(base::Value(callback_id), base::Value());
        return;
      }
    }
  }
  RejectJavascriptCallback(base::Value(callback_id),
                           base::Value("no session token"));
}

void BraveEmailAliasesHandler::RequestAccount(const base::Value::List& args) {
  CHECK_EQ(2U, args.size());
  const auto callback_id = args[0].GetString();
  const auto account_email = args[1].GetString();
  AllowJavascript();
  const auto bodyValue = base::Value::Dict()
                             .Set("email", account_email)
                             .Set("intent", "auth_token")
                             .Set("service", "email-aliases");
  ApiFetch(
      GURL(kAccountsServiceRequestURL), net::HttpRequestHeaders::kPostMethod,
      std::nullopt, bodyValue,
      base::BindOnce(&BraveEmailAliasesHandler::OnRequestAccountResponse,
                     weak_factory_.GetWeakPtr(), callback_id, account_email));
}

void BraveEmailAliasesHandler::OnRequestAccountResponse(
    const std::string& callback_id,
    const std::string& account_email,
    std::optional<std::string> response_body) {
  AllowJavascript();
  if (response_body) {
    std::optional<base::Value> response_value =
        base::JSONReader::Read(response_body.value());
    if (response_value && response_value->is_dict()) {
      const auto* verification_token =
          response_value->GetDict().Find("verificationToken");
      if (verification_token && verification_token->is_string()) {
        // Store the verification token while we wait for session confirmation.
        SetStringPref(kEmailAliasesAccountEmail, account_email);
        SetStringPref(kEmailAliasesVerificationToken,
                      verification_token->GetString());
        // Acknowledge success to the caller.
        ResolveJavascriptCallback(base::Value(callback_id), base::Value());
        return;
      }
    }
  }
  RejectJavascriptCallback(base::Value(callback_id),
                           base::Value("no verification token"));
}

void BraveEmailAliasesHandler::GetAccountEmail(const base::Value::List& args) {
  CHECK_EQ(1U, args.size());
  AllowJavascript();
  const auto callback_id = args[0].GetString();
  const auto account_email = GetStringPref(kEmailAliasesAccountEmail);
  ResolveJavascriptCallback(base::Value(callback_id),
                            base::Value(account_email));
}

void BraveEmailAliasesHandler::Logout(const base::Value::List& args) {
  AllowJavascript();
  ClearPref(kEmailAliasesAccountEmail);
  ClearPref(kEmailAliasesVerificationToken);
  ClearPref(kEmailAliasesAuthToken);
  ResolveJavascriptCallback(base::Value(args[0].GetString()), base::Value());
}

void BraveEmailAliasesHandler::CloseBubble(const base::Value::List& args) {
  CHECK_EQ(1U, args.size());
  email_aliases::EmailAliasesBubbleUI::Close();
}

void BraveEmailAliasesHandler::FillField(const base::Value::List& args) {
  CHECK_EQ(2U, args.size());
  AllowJavascript();
  const auto callback_id = args[0].GetString();
  const auto field_value = args[1].GetString();
  email_aliases::EmailAliasesBubbleUI::FillField(field_value);
  ResolveJavascriptCallback(base::Value(callback_id), base::Value());
}

void BraveEmailAliasesHandler::ShowSettingsPage(const base::Value::List& args) {
  CHECK_EQ(1U, args.size());
  ShowSingletonTabOverwritingNTP(GetProfile(), GURL(kEmailAliasesSettingsURL));
  email_aliases::EmailAliasesBubbleUI::Close();
}
