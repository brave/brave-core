/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_tab_helpers.h"

#include "brave/components/brave_rewards/browser/buildflags/buildflags.h"
#include "brave/components/brave_rewards/browser/payments_helper.h"
#include "brave/components/brave_shields/browser/brave_shields_web_contents_observer.h"
#include "content/public/browser/web_contents.h"

namespace brave {

void AttachTabHelpers(content::WebContents* web_contents) {
  brave_shields::BraveShieldsWebContentsObserver::CreateForWebContents(
      web_contents);
#if BUILDFLAG(BRAVE_PAYMENTS_ENABLED)
  payments::PaymentsHelper::CreateForWebContents(web_contents);
#endif
}

}  // namespace brave
