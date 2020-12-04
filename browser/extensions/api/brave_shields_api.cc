/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/api/brave_shields_api.h"

#include <utility>

#include "base/strings/string_number_conversions.h"
#include "brave/browser/brave_browser_process_impl.h"
#include "brave/browser/extensions/api/brave_action_api.h"
#include "brave/browser/webcompat_reporter/webcompat_reporter_dialog.h"
#include "brave/common/extensions/api/brave_shields.h"
#include "brave/components/brave_shields/browser/ad_block_base_service.h"
#include "brave/components/brave_shields/browser/ad_block_custom_filters_service.h"
#include "brave/components/brave_shields/browser/ad_block_regional_service_manager.h"
#include "brave/components/brave_shields/browser/ad_block_service.h"
#include "brave/components/brave_shields/browser/ad_block_service_helper.h"
#include "brave/components/brave_shields/browser/brave_shields_p3a.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "brave/components/brave_shields/browser/brave_shields_web_contents_observer.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/extensions/api/tabs/tabs_constants.h"
#include "chrome/browser/extensions/chrome_extension_function_details.h"
#include "chrome/browser/extensions/extension_tab_util.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/web_contents.h"
#include "extensions/browser/extension_util.h"
#include "extensions/common/constants.h"

using brave_shields::BraveShieldsWebContentsObserver;
using brave_shields::ControlType;
using brave_shields::ControlTypeFromString;
using brave_shields::ControlTypeToString;

namespace extensions {
namespace api {

namespace {

const char kInvalidUrlError[] = "Invalid URL.";
const char kInvalidControlTypeError[] = "Invalid ControlType.";

}  // namespace


ExtensionFunction::ResponseAction
BraveShieldsUrlCosmeticResourcesFunction::Run() {
  std::unique_ptr<brave_shields::UrlCosmeticResources::Params> params(
      brave_shields::UrlCosmeticResources::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());
  g_brave_browser_process->ad_block_service()->GetTaskRunner()
      ->PostTaskAndReplyWithResult(
          FROM_HERE,
          base::BindOnce(&BraveShieldsUrlCosmeticResourcesFunction::
                             GetUrlCosmeticResourcesOnTaskRunner,
                         this, params->url),
          base::BindOnce(&BraveShieldsUrlCosmeticResourcesFunction::
                             GetUrlCosmeticResourcesOnUI,
                         this));
  return RespondLater();
}

std::unique_ptr<base::ListValue> BraveShieldsUrlCosmeticResourcesFunction::
    GetUrlCosmeticResourcesOnTaskRunner(const std::string& url) {
  base::Optional<base::Value> resources = g_brave_browser_process->
      ad_block_service()->UrlCosmeticResources(url);

  if (!resources || !resources->is_dict()) {
    return std::unique_ptr<base::ListValue>();
  }

  base::Optional<base::Value> regional_resources = g_brave_browser_process->
      ad_block_regional_service_manager()->UrlCosmeticResources(url);

  if (regional_resources && regional_resources->is_dict()) {
    ::brave_shields::MergeResourcesInto(
        std::move(*regional_resources), &*resources, /*force_hide=*/false);
  }

  base::Optional<base::Value> custom_resources = g_brave_browser_process->
      ad_block_custom_filters_service()->UrlCosmeticResources(url);

  if (custom_resources && custom_resources->is_dict()) {
    ::brave_shields::MergeResourcesInto(
        std::move(*custom_resources), &*resources, /*force_hide=*/true);
  }

  auto result_list = std::make_unique<base::ListValue>();
  result_list->Append(std::move(*resources));
  return result_list;
}

void BraveShieldsUrlCosmeticResourcesFunction::GetUrlCosmeticResourcesOnUI(
    std::unique_ptr<base::ListValue> resources) {
  if (!resources) {
    Respond(Error("Url-specific cosmetic resources could not be returned"));
    return;
  }
  Respond(ArgumentList(std::move(resources)));
}

ExtensionFunction::ResponseAction
BraveShieldsHiddenClassIdSelectorsFunction::Run() {
  std::unique_ptr<brave_shields::HiddenClassIdSelectors::Params> params(
      brave_shields::HiddenClassIdSelectors::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());
  g_brave_browser_process->ad_block_service()->GetTaskRunner()
      ->PostTaskAndReplyWithResult(
          FROM_HERE,
          base::BindOnce(&BraveShieldsHiddenClassIdSelectorsFunction::
                             GetHiddenClassIdSelectorsOnTaskRunner,
                         this, params->classes, params->ids,
                         params->exceptions),
          base::BindOnce(&BraveShieldsHiddenClassIdSelectorsFunction::
                             GetHiddenClassIdSelectorsOnUI,
                         this));
  return RespondLater();
}

std::unique_ptr<base::ListValue> BraveShieldsHiddenClassIdSelectorsFunction::
    GetHiddenClassIdSelectorsOnTaskRunner(
        const std::vector<std::string>& classes,
        const std::vector<std::string>& ids,
        const std::vector<std::string>& exceptions) {
  base::Optional<base::Value> hide_selectors = g_brave_browser_process->
      ad_block_service()->HiddenClassIdSelectors(classes, ids, exceptions);

  base::Optional<base::Value> regional_selectors = g_brave_browser_process->
      ad_block_regional_service_manager()->
          HiddenClassIdSelectors(classes, ids, exceptions);

  base::Optional<base::Value> custom_selectors = g_brave_browser_process->
      ad_block_custom_filters_service()->
          HiddenClassIdSelectors(classes, ids, exceptions);

  if (hide_selectors && hide_selectors->is_list()) {
    if (regional_selectors && regional_selectors->is_list()) {
      for (auto i = regional_selectors->GetList().begin();
              i < regional_selectors->GetList().end();
              i++) {
        hide_selectors->Append(std::move(*i));
      }
    }
  } else {
    hide_selectors = std::move(regional_selectors);
  }

  auto result_list = std::make_unique<base::ListValue>();
  if (hide_selectors && hide_selectors->is_list()) {
    result_list->Append(std::move(*hide_selectors));
  }
  if (custom_selectors && custom_selectors->is_list()) {
    result_list->Append(std::move(*custom_selectors));
  }

  return result_list;
}

void BraveShieldsHiddenClassIdSelectorsFunction::
    GetHiddenClassIdSelectorsOnUI(std::unique_ptr<base::ListValue> selectors) {
  Respond(ArgumentList(std::move(selectors)));
}


ExtensionFunction::ResponseAction BraveShieldsAllowScriptsOnceFunction::Run() {
  std::unique_ptr<brave_shields::AllowScriptsOnce::Params> params(
      brave_shields::AllowScriptsOnce::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  // Get web contents for this tab
  content::WebContents* contents = nullptr;
  if (!ExtensionTabUtil::GetTabById(
          params->tab_id, Profile::FromBrowserContext(browser_context()),
          include_incognito_information(), nullptr, nullptr, &contents,
          nullptr)) {
    return RespondNow(Error(tabs_constants::kTabNotFoundError,
                            base::NumberToString(params->tab_id)));
  }

  BraveShieldsWebContentsObserver::FromWebContents(contents)->AllowScriptsOnce(
      params->origins, contents);
  return RespondNow(NoArguments());
}

BraveShieldsOpenBrowserActionUIFunction::
~BraveShieldsOpenBrowserActionUIFunction() {
}

ExtensionFunction::ResponseAction
BraveShieldsOpenBrowserActionUIFunction::Run() {
  std::unique_ptr<brave_shields::OpenBrowserActionUI::Params> params(
      brave_shields::OpenBrowserActionUI::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());
  std::string error;
  if (!BraveActionAPI::ShowActionUI(this,
      brave_extension_id,
      std::move(params->window_id),
      std::move(params->relative_path), &error)) {
    return RespondNow(Error(error));
  }
  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction
BraveShieldsSetBraveShieldsEnabledFunction::Run() {
  std::unique_ptr<brave_shields::SetBraveShieldsEnabled::Params> params(
      brave_shields::SetBraveShieldsEnabled::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  const GURL url(params->url);
  // we don't allow setting defaults from the extension
  if (url.is_empty() || !url.is_valid()) {
    return RespondNow(Error(kInvalidUrlError, params->url));
  }

  Profile* profile = Profile::FromBrowserContext(browser_context());
  ::brave_shields::SetBraveShieldsEnabled(
      HostContentSettingsMapFactory::GetForProfile(profile),
      params->enabled,
      url,
      g_browser_process->local_state());

  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction
BraveShieldsGetBraveShieldsEnabledFunction::Run() {
  std::unique_ptr<brave_shields::GetBraveShieldsEnabled::Params> params(
      brave_shields::GetBraveShieldsEnabled::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  const GURL url(params->url);
  // we don't allow getting defaults from the extension
  if (url.is_empty() || !url.is_valid()) {
    return RespondNow(Error(kInvalidUrlError, params->url));
  }

  Profile* profile = Profile::FromBrowserContext(browser_context());
  auto enabled = ::brave_shields::GetBraveShieldsEnabled(
      HostContentSettingsMapFactory::GetForProfile(profile),
      url);

  return RespondNow(OneArgument(base::Value(enabled)));
}

ExtensionFunction::ResponseAction
BraveShieldsShouldDoCosmeticFilteringFunction::Run() {
  std::unique_ptr<brave_shields::ShouldDoCosmeticFiltering::Params>
    params(
      brave_shields::ShouldDoCosmeticFiltering::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  const GURL url(params->url);
  // we don't allow getting defaults from the extension
  if (url.is_empty() || !url.is_valid()) {
    return RespondNow(Error(kInvalidUrlError, params->url));
  }

  Profile* profile = Profile::FromBrowserContext(browser_context());
  const bool enabled = ::brave_shields::ShouldDoCosmeticFiltering(
      HostContentSettingsMapFactory::GetForProfile(profile),
      url);

  return RespondNow(OneArgument(base::Value(enabled)));
}

ExtensionFunction::ResponseAction
BraveShieldsSetCosmeticFilteringControlTypeFunction::Run() {
  std::unique_ptr<brave_shields::SetCosmeticFilteringControlType::Params>
    params(
      brave_shields::SetCosmeticFilteringControlType::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  const GURL url(params->url);
  // we don't allow setting defaults from the extension
  if (url.is_empty() || !url.is_valid()) {
    return RespondNow(Error(kInvalidUrlError, params->url));
  }

  auto control_type = ControlTypeFromString(params->control_type);
  if (control_type == ControlType::INVALID) {
    return RespondNow(Error(kInvalidControlTypeError, params->control_type));
  }

  Profile* profile = Profile::FromBrowserContext(browser_context());
  ::brave_shields::SetCosmeticFilteringControlType(
      HostContentSettingsMapFactory::GetForProfile(profile),
      control_type,
      url,
      g_browser_process->local_state());

  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction
BraveShieldsIsFirstPartyCosmeticFilteringEnabledFunction::Run() {
  std::unique_ptr<brave_shields::IsFirstPartyCosmeticFilteringEnabled::Params>
      params(
          brave_shields::IsFirstPartyCosmeticFilteringEnabled::Params::Create(
          *args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  const GURL url(params->url);
  // we don't allow getting defaults from the extension
  if (url.is_empty() || !url.is_valid()) {
    return RespondNow(Error(kInvalidUrlError, params->url));
  }

  Profile* profile = Profile::FromBrowserContext(browser_context());
  const bool enabled = ::brave_shields::IsFirstPartyCosmeticFilteringEnabled(
      HostContentSettingsMapFactory::GetForProfile(profile), url);

  return RespondNow(OneArgument(base::Value(enabled)));
}

ExtensionFunction::ResponseAction BraveShieldsSetAdControlTypeFunction::Run() {
  std::unique_ptr<brave_shields::SetAdControlType::Params> params(
      brave_shields::SetAdControlType::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  const GURL url(params->url);
  // we don't allow setting defaults from the extension
  if (url.is_empty() || !url.is_valid()) {
    return RespondNow(Error(kInvalidUrlError, params->url));
  }

  auto control_type = ControlTypeFromString(params->control_type);
  if (control_type == ControlType::INVALID) {
    return RespondNow(Error(kInvalidControlTypeError, params->control_type));
  }

  Profile* profile = Profile::FromBrowserContext(browser_context());
  ::brave_shields::SetAdControlType(
      HostContentSettingsMapFactory::GetForProfile(profile),
      control_type,
      url,
      g_browser_process->local_state());

  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction BraveShieldsGetAdControlTypeFunction::Run() {
  std::unique_ptr<brave_shields::GetAdControlType::Params> params(
      brave_shields::GetAdControlType::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  const GURL url(params->url);
  // we don't allow getting defaults from the extension
  if (url.is_empty() || !url.is_valid()) {
    return RespondNow(Error(kInvalidUrlError, params->url));
  }

  Profile* profile = Profile::FromBrowserContext(browser_context());
  auto type = ::brave_shields::GetAdControlType(
      HostContentSettingsMapFactory::GetForProfile(profile),
      url);

  return RespondNow(OneArgument(base::Value(ControlTypeToString(type))));
}

ExtensionFunction::ResponseAction
BraveShieldsSetCookieControlTypeFunction::Run() {
  std::unique_ptr<brave_shields::SetCookieControlType::Params> params(
      brave_shields::SetCookieControlType::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  const GURL url(params->url);
  // we don't allow setting defaults from the extension
  if (url.is_empty() || !url.is_valid()) {
    return RespondNow(Error(kInvalidUrlError, params->url));
  }

  auto control_type = ControlTypeFromString(params->control_type);
  if (control_type == ControlType::INVALID) {
    return RespondNow(Error(kInvalidControlTypeError, params->control_type));
  }

  Profile* profile = Profile::FromBrowserContext(browser_context());
  ::brave_shields::SetCookieControlType(
      HostContentSettingsMapFactory::GetForProfile(profile),
      control_type,
      url,
      g_browser_process->local_state());

  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction
BraveShieldsGetCookieControlTypeFunction::Run() {
  std::unique_ptr<brave_shields::GetCookieControlType::Params> params(
      brave_shields::GetCookieControlType::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  const GURL url(params->url);
  // we don't allow getting defaults from the extension
  if (url.is_empty() || !url.is_valid()) {
    return RespondNow(Error(kInvalidUrlError, params->url));
  }

  Profile* profile = Profile::FromBrowserContext(browser_context());
  auto type = ::brave_shields::GetCookieControlType(
      HostContentSettingsMapFactory::GetForProfile(profile),
      url);

  return RespondNow(OneArgument(base::Value(ControlTypeToString(type))));
}

ExtensionFunction::ResponseAction
BraveShieldsSetFingerprintingControlTypeFunction::Run() {
  std::unique_ptr<brave_shields::SetFingerprintingControlType::Params> params(
      brave_shields::SetFingerprintingControlType::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  const GURL url(params->url);
  // we don't allow setting defaults from the extension
  if (url.is_empty() || !url.is_valid()) {
    return RespondNow(Error(kInvalidUrlError, params->url));
  }

  auto control_type = ControlTypeFromString(params->control_type);
  if (control_type == ControlType::INVALID) {
    return RespondNow(Error(kInvalidControlTypeError, params->control_type));
  }

  Profile* profile = Profile::FromBrowserContext(browser_context());
  ::brave_shields::SetFingerprintingControlType(
      HostContentSettingsMapFactory::GetForProfile(profile),
      control_type,
      url,
      g_browser_process->local_state());

  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction
BraveShieldsGetFingerprintingControlTypeFunction::Run() {
  std::unique_ptr<brave_shields::GetFingerprintingControlType::Params> params(
      brave_shields::GetFingerprintingControlType::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  const GURL url(params->url);
  // we don't allow getting defaults from the extension
  if (url.is_empty() || !url.is_valid()) {
    return RespondNow(Error(kInvalidUrlError, params->url));
  }

  Profile* profile = Profile::FromBrowserContext(browser_context());
  auto type = ::brave_shields::GetFingerprintingControlType(
      HostContentSettingsMapFactory::GetForProfile(profile),
      url);

  return RespondNow(OneArgument(base::Value(ControlTypeToString(type))));
}

ExtensionFunction::ResponseAction
BraveShieldsSetHTTPSEverywhereEnabledFunction::Run() {
  std::unique_ptr<brave_shields::SetHTTPSEverywhereEnabled::Params> params(
      brave_shields::SetHTTPSEverywhereEnabled::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  const GURL url(params->url);
  // we don't allow setting defaults from the extension
  if (url.is_empty() || !url.is_valid()) {
    return RespondNow(Error(kInvalidUrlError, params->url));
  }

  Profile* profile = Profile::FromBrowserContext(browser_context());
  ::brave_shields::SetHTTPSEverywhereEnabled(
      HostContentSettingsMapFactory::GetForProfile(profile),
      params->enabled,
      url,
      g_browser_process->local_state());

  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction
BraveShieldsGetHTTPSEverywhereEnabledFunction::Run() {
  std::unique_ptr<brave_shields::GetHTTPSEverywhereEnabled::Params> params(
      brave_shields::GetHTTPSEverywhereEnabled::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  const GURL url(params->url);
  // we don't allow getting defaults from the extension
  if (url.is_empty() || !url.is_valid()) {
    return RespondNow(Error(kInvalidUrlError, params->url));
  }

  Profile* profile = Profile::FromBrowserContext(browser_context());
  auto type = ::brave_shields::GetHTTPSEverywhereEnabled(
      HostContentSettingsMapFactory::GetForProfile(profile),
      url);

  return RespondNow(OneArgument(base::Value(type)));
}

ExtensionFunction::ResponseAction
BraveShieldsSetNoScriptControlTypeFunction::Run() {
  std::unique_ptr<brave_shields::SetNoScriptControlType::Params> params(
      brave_shields::SetNoScriptControlType::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  const GURL url(params->url);
  // we don't allow setting defaults from the extension
  if (url.is_empty() || !url.is_valid()) {
    return RespondNow(Error(kInvalidUrlError, params->url));
  }

  auto control_type = ControlTypeFromString(params->control_type);
  if (control_type == ControlType::INVALID) {
    return RespondNow(Error(kInvalidControlTypeError, params->control_type));
  }

  Profile* profile = Profile::FromBrowserContext(browser_context());
  ::brave_shields::SetNoScriptControlType(
      HostContentSettingsMapFactory::GetForProfile(profile),
      control_type,
      url,
      g_browser_process->local_state());

  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction
BraveShieldsGetNoScriptControlTypeFunction::Run() {
  std::unique_ptr<brave_shields::GetNoScriptControlType::Params> params(
      brave_shields::GetNoScriptControlType::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  const GURL url(params->url);
  // we don't allow getting defaults from the extension
  if (url.is_empty() || !url.is_valid()) {
    return RespondNow(Error(kInvalidUrlError, params->url));
  }

  Profile* profile = Profile::FromBrowserContext(browser_context());
  auto type = ::brave_shields::GetNoScriptControlType(
      HostContentSettingsMapFactory::GetForProfile(profile),
      url);

  return RespondNow(OneArgument(base::Value(ControlTypeToString(type))));
}

ExtensionFunction::ResponseAction
BraveShieldsOnShieldsPanelShownFunction::Run() {
  ::brave_shields::MaybeRecordShieldsUsageP3A(::brave_shields::kClicked,
                                              g_browser_process->local_state());
  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction BraveShieldsReportBrokenSiteFunction::Run() {
  std::unique_ptr<brave_shields::ReportBrokenSite::Params> params(
      brave_shields::ReportBrokenSite::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  // Get web contents for this tab
  content::WebContents* contents = nullptr;
  if (!ExtensionTabUtil::GetTabById(
        params->tab_id,
        Profile::FromBrowserContext(browser_context()),
        false,
        nullptr,
        nullptr,
        &contents,
        nullptr)) {
    return RespondNow(Error(tabs_constants::kTabNotFoundError,
                            base::NumberToString(params->tab_id)));
  }

  OpenWebcompatReporterDialog(contents);

  return RespondNow(NoArguments());
}

}  // namespace api
}  // namespace extensions
