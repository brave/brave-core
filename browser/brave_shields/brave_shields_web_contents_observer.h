/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_SHIELDS_BRAVE_SHIELDS_WEB_CONTENTS_OBSERVER_H_
#define BRAVE_BROWSER_BRAVE_SHIELDS_BRAVE_SHIELDS_WEB_CONTENTS_OBSERVER_H_

#include <map>
#include <set>
#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/synchronization/lock.h"
#include "brave/components/brave_shields/common/brave_shields.mojom.h"
#include "content/public/browser/render_frame_host_receiver_set.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

namespace content {
class WebContents;
}

class PrefRegistrySimple;

namespace brave_shields {

class BraveShieldsWebContentsObserver
    : public content::WebContentsObserver,
      public content::WebContentsUserData<BraveShieldsWebContentsObserver>,
      public brave_shields::mojom::BraveShieldsHost {
 public:
  explicit BraveShieldsWebContentsObserver(content::WebContents*);
  BraveShieldsWebContentsObserver(const BraveShieldsWebContentsObserver&) =
      delete;
  BraveShieldsWebContentsObserver& operator=(
      const BraveShieldsWebContentsObserver&) = delete;
  ~BraveShieldsWebContentsObserver() override;

  static void BindBraveShieldsHost(
      mojo::PendingAssociatedReceiver<brave_shields::mojom::BraveShieldsHost>
          receiver,
      content::RenderFrameHost* rfh);

  static void RegisterProfilePrefs(PrefRegistrySimple* registry);
  static void DispatchBlockedEventForWebContents(
      const std::string& block_type,
      const std::string& subresource,
      content::WebContents* web_contents);
  static void DispatchBlockedEvent(const GURL& request_url,
                                   int frame_tree_node_id,
                                   const std::string& block_type);
  static GURL GetTabURLFromRenderFrameInfo(int render_frame_tree_node_id);
  void AllowScriptsOnce(const std::vector<std::string>& origins,
                        content::WebContents* web_contents);
  bool IsBlockedSubresource(const std::string& subresource);
  void AddBlockedSubresource(const std::string& subresource);

 protected:
  // content::WebContentsObserver overrides.
  void RenderFrameCreated(content::RenderFrameHost* host) override;
  void RenderFrameDeleted(content::RenderFrameHost* render_frame_host) override;
  void RenderFrameHostChanged(content::RenderFrameHost* old_host,
                              content::RenderFrameHost* new_host) override;
  void ReadyToCommitNavigation(
      content::NavigationHandle* navigation_handle) override;

  // brave_shields::mojom::BraveShieldsHost.
  void OnJavaScriptBlocked(const std::u16string& details) override;

 private:
  friend class content::WebContentsUserData<BraveShieldsWebContentsObserver>;
  friend class BraveShieldsWebContentsObserverBrowserTest;

  using BraveShieldsRemotesMap = base::flat_map<
      content::RenderFrameHost*,
      mojo::AssociatedRemote<brave_shields::mojom::BraveShields>>;

  // Allows indicating a implementor of brave_shields::mojom::BraveShieldsHost
  // other than this own class, for testing purposes only.
  static void SetReceiverImplForTesting(BraveShieldsWebContentsObserver* impl);

  // Only used from the BindBraveShieldsHost() static method, useful to bind the
  // mojo receiver of brave_shields::mojom::BraveShieldsHost to a different
  // implementor when needed, for testing purposes.
  void BindReceiver(mojo::PendingAssociatedReceiver<
                        brave_shields::mojom::BraveShieldsHost> receiver,
                    content::RenderFrameHost* rfh);

  // Return an already bound remote for the brave_shields::mojom::BraveShields
  // mojo interface. It is an error to call this method with an invalid |rfh|.
  mojo::AssociatedRemote<brave_shields::mojom::BraveShields>&
  GetBraveShieldsRemote(content::RenderFrameHost* rfh);

  std::vector<std::string> allowed_script_origins_;
  // We keep a set of the current page's blocked URLs in case the page
  // continually tries to load the same blocked URLs.
  std::set<std::string> blocked_url_paths_;

  content::RenderFrameHostReceiverSet<brave_shields::mojom::BraveShieldsHost>
      receivers_;

  // Map of remote endpoints for the brave_shields::mojom::BraveShields mojo
  // interface, to prevent binding a new remote each time it's used.
  BraveShieldsRemotesMap brave_shields_remotes_;

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace brave_shields

#endif  // BRAVE_BROWSER_BRAVE_SHIELDS_BRAVE_SHIELDS_WEB_CONTENTS_OBSERVER_H_
