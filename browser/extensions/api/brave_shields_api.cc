/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/api/brave_shields_api.h"

#include <memory>
#include <string>
#include <utility>

#include "base/strings/string_number_conversions.h"
#include "brave/common/extensions/api/brave_shields.h"
#include "brave/common/extensions/extension_constants.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "brave/components/brave_shields/browser/brave_shields_web_contents_observer.h"
#include "chrome/browser/extensions/api/tabs/tabs_constants.h"
#include "chrome/browser/extensions/extension_tab_util.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/web_contents.h"
#include "extensions/browser/extension_util.h"
#include "brave/components/brave_shields/browser/ad_block_base_service.h"
#include "brave/components/brave_shields/browser/ad_block_service.h"
#include "brave/browser/brave_browser_process_impl.h"
#include "brave/vendor/adblock_rust_ffi/src/wrapper.hpp"

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


ExtensionFunction::ResponseAction BraveShieldsHostnameCosmeticResourcesFunction::Run() {
  std::unique_ptr<brave_shields::HostnameCosmeticResources::Params> params(
      brave_shields::HostnameCosmeticResources::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  adblock::HostnameResources resources = g_brave_browser_process->ad_block_service()->hostnameCosmeticResources(params->hostname);

  auto result_list = std::make_unique<base::ListValue>();

  result_list->GetList().push_back(base::Value(resources.stylesheet));

  base::Value exceptions(base::Value::Type::LIST);
  for(auto i = resources.exceptions.begin(); i != resources.exceptions.end(); i++) {
    exceptions.GetList().push_back(base::Value(*i));
  }
  result_list->GetList().push_back(std::move(exceptions));

  result_list->GetList().push_back(base::Value(resources.injected_script));

  return RespondNow(ArgumentList(std::move(result_list)));
}

ExtensionFunction::ResponseAction BraveShieldsClassIdStylesheetFunction::Run() {
  std::unique_ptr<brave_shields::ClassIdStylesheet::Params> params(
      brave_shields::ClassIdStylesheet::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  std::string stylesheet = g_brave_browser_process->ad_block_service()->classIdStylesheet(params->classes, params->ids, params->exceptions);
  return RespondNow(OneArgument(std::make_unique<base::Value>(stylesheet)));
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
  ::brave_shields::SetBraveShieldsEnabled(profile, params->enabled, url);

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
  auto enabled = ::brave_shields::GetBraveShieldsEnabled(profile, url);
  auto result = std::make_unique<base::Value>(enabled);

  return RespondNow(OneArgument(std::move(result)));
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
  ::brave_shields::SetAdControlType(profile, control_type, url);

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
  auto type = ::brave_shields::GetAdControlType(profile, url);
  auto result = std::make_unique<base::Value>(ControlTypeToString(type));

  return RespondNow(OneArgument(std::move(result)));
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
  ::brave_shields::SetCookieControlType(profile, control_type, url);

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
  auto type = ::brave_shields::GetCookieControlType(profile, url);
  auto result = std::make_unique<base::Value>(ControlTypeToString(type));

  return RespondNow(OneArgument(std::move(result)));
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
  ::brave_shields::SetFingerprintingControlType(profile, control_type, url);

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
  auto type = ::brave_shields::GetFingerprintingControlType(profile, url);
  auto result = std::make_unique<base::Value>(ControlTypeToString(type));

  return RespondNow(OneArgument(std::move(result)));
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
  ::brave_shields::SetHTTPSEverywhereEnabled(profile, params->enabled, url);

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
  auto type = ::brave_shields::GetHTTPSEverywhereEnabled(profile, url);
  auto result = std::make_unique<base::Value>(type);

  return RespondNow(OneArgument(std::move(result)));
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
  ::brave_shields::SetNoScriptControlType(profile, control_type, url);

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
  auto type = ::brave_shields::GetNoScriptControlType(profile, url);
  auto result = std::make_unique<base::Value>(ControlTypeToString(type));

  return RespondNow(OneArgument(std::move(result)));
}

}  // namespace api
}  // namespace extensions
