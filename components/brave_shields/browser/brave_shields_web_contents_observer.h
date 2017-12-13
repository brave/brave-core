/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_H_

#include "base/macros.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

namespace content {
class WebContents;
}

namespace brave_shields {

class BraveShieldsWebContentsObserver : public content::WebContentsObserver,
    public content::WebContentsUserData<BraveShieldsWebContentsObserver> {
 public:
  BraveShieldsWebContentsObserver(content::WebContents*);
  ~BraveShieldsWebContentsObserver() override;

  // content::WebContentsObserver overrides.
  void RenderFrameCreated(content::RenderFrameHost* host) override;

  DISALLOW_COPY_AND_ASSIGN(BraveShieldsWebContentsObserver);
};

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_H_
