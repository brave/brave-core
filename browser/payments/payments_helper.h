/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_PAYMENTS_PAYMENTS_HELPER_
#define BRAVE_BROWSER_PAYMENTS_PAYMENTS_HELPER_

#include <string>

#include "base/macros.h"
#include "build/build_config.h"
#include "chrome/browser/ui/browser_list_observer.h"
#include "components/sessions/core/session_id.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

class Browser;

namespace payments {

class PaymentsService;

class PaymentsHelper : public content::WebContentsObserver,
                       public BrowserListObserver,
                       public content::WebContentsUserData<PaymentsHelper> {
 public:
  PaymentsHelper(content::WebContents*);
  ~PaymentsHelper() override;

 private:
  friend class content::WebContentsUserData<PaymentsHelper>;

  // content::WebContentsObserver overrides.
  void DidFinishLoad(content::RenderFrameHost* render_frame_host,
                     const GURL& validated_url) override;
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;
  void ResourceLoadComplete(
      content::RenderFrameHost* render_frame_host,
      const content::GlobalRequestID& request_id,
      const content::mojom::ResourceLoadInfo& resource_load_info) override;
  void DidAttachInterstitialPage() override;
  void MediaStartedPlaying(const MediaPlayerInfo& video_type,
                           const MediaPlayerId& id) override;
  void MediaStoppedPlaying(
      const MediaPlayerInfo& video_type,
      const MediaPlayerId& id,
      WebContentsObserver::MediaStoppedReason reason) override;
  void OnVisibilityChanged(content::Visibility visibility) override;
  void WebContentsDestroyed() override;

  // BrowserListObserver overrides
  void OnBrowserSetLastActive(Browser* browser) override;
  void OnBrowserNoLongerActive(Browser* browser) override;

  SessionID tab_id_;
  PaymentsService* payments_service_;  // NOT OWNED

  DISALLOW_COPY_AND_ASSIGN(PaymentsHelper);
};

}  // namespace brave_shields

#endif  // BRAVE_BROWSER_PAYMENTS_PAYMENTS_HELPER_
