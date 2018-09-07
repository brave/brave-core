/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/browser_commands.h"

#include "brave/browser/tor/tor_profile_service.h"
#include "brave/browser/tor/tor_profile_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/web_contents.h"

using content::BrowserThread;
using content::NavigationController;
using content::WebContents;

namespace {
void NewTorIdentityCallback(WebContents* current_tab) {
  NavigationController& controller = current_tab->GetController();
  controller.Reload(content::ReloadType::BYPASSING_CACHE, true);
}
}  // namespace

namespace brave {

void NewTorIdentity(Browser* browser) {
  Profile* profile = browser->profile();
  DCHECK(profile);
  tor::TorProfileService* service =
    TorProfileServiceFactory::GetForProfile(profile);
  DCHECK(service);
  WebContents* current_tab =
    browser->tab_strip_model()->GetActiveWebContents();
  if (!current_tab)
    return;
  const GURL current_url = current_tab->GetURL();
  service->SetNewTorCircuit(current_url, base::Bind(&NewTorIdentityCallback,
                                                    current_tab));
}

}  // namespace brave
