/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/api/brave_shields_api.h"

#include "brave/common/extensions/api/brave_shields.h"
#include "brave/components/brave_shields/browser/brave_shields_web_contents_observer.h"
#include "chrome/browser/extensions/api/tabs/tabs_constants.h"
#include "chrome/browser/extensions/extension_tab_util.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/web_contents.h"

using brave_shields::BraveShieldsWebContentsObserver;

namespace extensions {
namespace api {

BraveShieldsAllowScriptsOnceFunction::~BraveShieldsAllowScriptsOnceFunction() {
}

ExtensionFunction::ResponseAction BraveShieldsAllowScriptsOnceFunction::Run() {
  std::unique_ptr<brave_shields::AllowScriptsOnce::Params> params(
      brave_shields::AllowScriptsOnce::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  // Get web contents for this tab
  content::WebContents* contents = nullptr;
  if (!ExtensionTabUtil::GetTabById(
        params->tab_id,
        Profile::FromBrowserContext(browser_context()),
        include_incognito_information(),
        nullptr,
        nullptr,
        &contents,
        nullptr)) {
    return RespondNow(Error(tabs_constants::kTabNotFoundError,
                            base::IntToString(params->tab_id)));
  }

  BraveShieldsWebContentsObserver::FromWebContents(
      contents)->AllowScriptsOnce(params->origins, contents);
  return RespondNow(NoArguments());
}

}  // namespace api
}  // namespace extensions
