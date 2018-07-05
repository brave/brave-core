/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_welcome_ui.h"

#include "brave/browser/brave_browser_process_impl.h"
#include "brave/common/pref_names.h"
#include "brave/common/webui_url_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/pref_names.h"
#include "components/grit/brave_components_resources.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_service.h"

BraveWelcomeUI::BraveWelcomeUI(content::WebUI* web_ui, const std::string& name)
    : BasicUI(web_ui, name, kWelcomeJS,
        IDR_BRAVE_WELCOME_JS, IDR_BRAVE_WELCOME_HTML) {
  Profile* profile = Profile::FromWebUI(web_ui);

  profile->GetPrefs()->SetBoolean(prefs::kHasSeenWelcomePage, true);
#if defined(OS_WIN)
  g_brave_browser_process->local_state()->SetBoolean(
      prefs::kHasSeenWin10PromoPage,
      true);
#endif
}

BraveWelcomeUI::~BraveWelcomeUI() {
}
