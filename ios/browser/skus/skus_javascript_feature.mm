// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/skus/skus_javascript_feature.h"

#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "base/check.h"
#include "base/containers/fixed_flat_set.h"
#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "base/memory/ptr_util.h"
#include "base/values.h"
#include "brave/ios/browser/skus/skus_service_factory.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/web/public/browser_state.h"
#include "ios/web/public/js_messaging/origin_filter.h"
#include "ios/web/public/js_messaging/script_message.h"
#include "ios/web/public/web_state.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "url/gurl.h"
#include "url/url_constants.h"

namespace skus {

namespace {

constexpr char kSkusJavaScriptFeatureKeyName[] =
    "brave_skus_javascript_feature";
constexpr char kScriptName[] = "skus_helper";
constexpr char kScriptHandlerName[] = "BraveSkusMessageHandler";

constexpr char kMethodIdKey[] = "method_id";
constexpr char kDataKey[] = "data";
constexpr char kOrderIdKey[] = "orderId";
constexpr char kDomainKey[] = "domain";
constexpr char kPathKey[] = "path";

constexpr char kMethodRefreshOrder[] = "refreshOrder";
constexpr char kMethodFetchOrderCredentials[] = "fetchOrderCredentials";
constexpr char kMethodPrepareCredentialsPresentation[] =
    "prepareCredentialsPresentation";
constexpr char kMethodCredentialsSummary[] = "credentialsSummary";

using ScriptMessageReplyCallback =
    base::OnceCallback<void(const base::Value* reply, NSString* error)>;

// Resolves `callback` with `response->message` parsed as JSON, or with null if
// parsing fails.
void ReplyWithJsonResult(ScriptMessageReplyCallback callback,
                         mojom::SkusResultPtr response) {
  std::optional<base::Value> parsed = base::JSONReader::Read(
      response->message, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                             base::JSONParserOptions::JSON_PARSE_RFC);
  if (!parsed) {
    std::move(callback).Run(nullptr, nil);
    return;
  }
  std::move(callback).Run(&*parsed, nil);
}

// Resolves `callback` with `response->message` as a raw string.
void ReplyWithStringResult(ScriptMessageReplyCallback callback,
                           mojom::SkusResultPtr response) {
  base::Value value(response->message);
  std::move(callback).Run(&value, nil);
}

}  // namespace

SkusJavaScriptFeature::SkusJavaScriptFeature(ProfileIOS* profile)
    : JavaScriptFeature(web::ContentWorld::kPageContentWorld,
                        /*feature_scripts=*/{},
                        /*dependent_feature=*/{},
                        web::OriginFilter::kBraveAccount),
      profile_(profile) {}

SkusJavaScriptFeature::~SkusJavaScriptFeature() = default;

// static
SkusJavaScriptFeature* SkusJavaScriptFeature::FromBrowserState(
    web::BrowserState* browser_state) {
  DCHECK(browser_state);
  SkusJavaScriptFeature* feature = static_cast<SkusJavaScriptFeature*>(
      browser_state->GetUserData(kSkusJavaScriptFeatureKeyName));
  if (!feature) {
    feature =
        new SkusJavaScriptFeature(ProfileIOS::FromBrowserState(browser_state));
    browser_state->SetUserData(kSkusJavaScriptFeatureKeyName,
                               base::WrapUnique(feature));
  }
  return feature;
}

bool SkusJavaScriptFeature::GetFeatureRepliesToMessages() const {
  return true;
}

std::optional<std::string> SkusJavaScriptFeature::GetScriptMessageHandlerName()
    const {
  return kScriptHandlerName;
}

std::vector<web::JavaScriptFeature::FeatureScript>
SkusJavaScriptFeature::GetScripts() const {
  if (profile_->IsOffTheRecord()) {
    // Dont inject skus script when in private windows
    return {};
  }
  return {FeatureScript::CreateWithFilename(
      kScriptName, FeatureScript::InjectionTime::kDocumentStart,
      FeatureScript::TargetFrames::kMainFrame,
      FeatureScript::ReinjectionBehavior::kInjectOncePerWindow,
      FeatureScript::PlaceholderReplacementsCallback(),
      web::OriginFilter::kBraveAccount)};
}

void SkusJavaScriptFeature::SetCredentialSummaryFetched(
    CredentialSummaryFetchedCallback callback) {
  credential_summary_fetched_ = std::move(callback);
}

void SkusJavaScriptFeature::EnsureMojoConnected() {
  if (skus_service_.is_bound()) {
    return;
  }
  mojo::PendingRemote<mojom::SkusService> pending =
      SkusServiceFactory::GetForProfile(profile_);
  if (!pending) {
    return;
  }
  skus_service_.Bind(std::move(pending));
  skus_service_.set_disconnect_handler(base::BindOnce(
      &SkusJavaScriptFeature::OnMojoConnectionError, base::Unretained(this)));
}

void SkusJavaScriptFeature::OnMojoConnectionError() {
  skus_service_.reset();
}

void SkusJavaScriptFeature::OnCredentialSummary(
    const std::string domain,
    ScriptMessageReplyCallback callback,
    mojom::SkusResultPtr response) {
  if (credential_summary_fetched_) {
    credential_summary_fetched_.Run(domain, response->message);
  }
  ReplyWithJsonResult(std::move(callback), std::move(response));
}

void SkusJavaScriptFeature::ScriptMessageReceivedWithReply(
    web::WebState* web_state,
    const web::ScriptMessage& message,
    ScriptMessageReplyCallback callback) {
  url::Origin security_origin = message.security_origin();
  if (!message.is_main_frame() || security_origin.opaque()) {
    std::move(callback).Run(nullptr, nil);
    return;
  }

  const base::DictValue* body =
      message.legacy_body() ? message.legacy_body()->GetIfDict() : nullptr;
  if (!body) {
    std::move(callback).Run(nullptr, nil);
    return;
  }

  const std::string* method_id = body->FindString(kMethodIdKey);
  const base::DictValue* data = body->FindDict(kDataKey);
  if (!method_id || !data) {
    std::move(callback).Run(nullptr, nil);
    return;
  }

  EnsureMojoConnected();
  if (!skus_service_.is_bound()) {
    std::move(callback).Run(nullptr, nil);
    return;
  }

  const std::string host = security_origin.host();

  if (*method_id == kMethodRefreshOrder) {
    const std::string* order_id = data->FindString(kOrderIdKey);
    if (!order_id) {
      std::move(callback).Run(nullptr, nil);
      return;
    }
    skus_service_->RefreshOrder(
        host, *order_id,
        base::BindOnce(&ReplyWithJsonResult, std::move(callback)));
    return;
  }

  if (*method_id == kMethodFetchOrderCredentials) {
    const std::string* order_id = data->FindString(kOrderIdKey);
    if (!order_id) {
      std::move(callback).Run(nullptr, nil);
      return;
    }
    skus_service_->FetchOrderCredentials(
        host, *order_id,
        base::BindOnce(&ReplyWithStringResult, std::move(callback)));
    return;
  }

  if (*method_id == kMethodPrepareCredentialsPresentation) {
    const std::string* domain = data->FindString(kDomainKey);
    const std::string* path = data->FindString(kPathKey);
    if (!domain || !path) {
      std::move(callback).Run(nullptr, nil);
      return;
    }
    skus_service_->PrepareCredentialsPresentation(
        *domain, *path,
        base::BindOnce(&ReplyWithStringResult, std::move(callback)));
    return;
  }

  if (*method_id == kMethodCredentialsSummary) {
    const std::string* domain = data->FindString(kDomainKey);
    if (!domain) {
      std::move(callback).Run(nullptr, nil);
      return;
    }
    skus_service_->CredentialSummary(
        *domain,
        base::BindOnce(&SkusJavaScriptFeature::OnCredentialSummary,
                       base::Unretained(this), *domain, std::move(callback)));
    return;
  }

  std::move(callback).Run(nullptr, nil);
}

}  // namespace skus
