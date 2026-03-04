/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/api/web_discovery_api.h"

#include <utility>

#include "brave/browser/brave_search/backup_results_service_factory.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/web_discovery/buildflags/buildflags.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "extensions/browser/extension_function.h"

#if BUILDFLAG(ENABLE_WEB_DISCOVERY_NATIVE)
#include "base/feature_list.h"
#include "brave/components/web_discovery/common/features.h"
#endif

namespace extensions::api {

namespace {

constexpr char kResponseCodeKey[] = "responseCode";
constexpr char kHtmlKey[] = "html";

}  // namespace

WebDiscoveryRetrieveBackupResultsFunction::
    ~WebDiscoveryRetrieveBackupResultsFunction() = default;

ExtensionFunction::ResponseAction
WebDiscoveryRetrieveBackupResultsFunction::Run() {
  auto* user_prefs = user_prefs::UserPrefs::Get(browser_context());
  if (!user_prefs || !user_prefs->GetBoolean(kWebDiscoveryEnabled)) {
    return RespondNow(Error("web discovery is not enabled"));
  }
  EXTENSION_FUNCTION_VALIDATE(has_args() && !args().empty());
  const auto* url_str = args().front().GetIfString();
  EXTENSION_FUNCTION_VALIDATE(url_str);
  auto* service =
      brave_search::BackupResultsServiceFactory::GetForBrowserContext(
          browser_context());
  if (!service) {
    return RespondNow(Error("failed to get BackupResultsService"));
  }
  service->FetchBackupResults(
      GURL(*url_str), std::nullopt,
      base::BindOnce(
          &WebDiscoveryRetrieveBackupResultsFunction::HandleBackupResults,
          this));
  return RespondLater();
}

void WebDiscoveryRetrieveBackupResultsFunction::HandleBackupResults(
    std::optional<brave_search::BackupResultsService::BackupResults> results) {
  if (!results) {
    Respond(Error("failed to retrieve backup results"));
    return;
  }

  base::DictValue result_dict;
  result_dict.Set(kResponseCodeKey, results->final_status_code);
  result_dict.Set(kHtmlKey, std::move(results->html));

  Respond(WithArguments(base::Value(std::move(result_dict))));
}

WebDiscoveryIsWebDiscoveryExtensionEnabledFunction::
    ~WebDiscoveryIsWebDiscoveryExtensionEnabledFunction() = default;

ExtensionFunction::ResponseAction
WebDiscoveryIsWebDiscoveryExtensionEnabledFunction::Run() {
  bool native_enabled = false;
#if BUILDFLAG(ENABLE_WEB_DISCOVERY_NATIVE)
  native_enabled = base::FeatureList::IsEnabled(
      web_discovery::features::kBraveWebDiscoveryNative);
#endif

  auto* user_prefs = user_prefs::UserPrefs::Get(browser_context());
  bool result = !native_enabled && user_prefs &&
                user_prefs->GetBoolean(kWebDiscoveryEnabled);

  return RespondNow(WithArguments(result));
}

}  // namespace extensions::api
