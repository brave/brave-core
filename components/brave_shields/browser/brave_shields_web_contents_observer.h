/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_BRAVE_SHIELDS_WEB_CONTENTS_OBSERVER_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_BRAVE_SHIELDS_WEB_CONTENTS_OBSERVER_H_

#include <map>
#include <set>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/synchronization/lock.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

namespace content {
class WebContents;
}

class PrefRegistrySimple;

namespace brave_shields {

class BraveShieldsWebContentsObserver : public content::WebContentsObserver,
    public content::WebContentsUserData<BraveShieldsWebContentsObserver> {
 public:
  explicit BraveShieldsWebContentsObserver(content::WebContents*);
  ~BraveShieldsWebContentsObserver() override;

  static void RegisterProfilePrefs(PrefRegistrySimple* registry);
  static void DispatchBlockedEventForWebContents(
      const std::string& block_type,
      const std::string& subresource,
      content::WebContents* web_contents);
  static void DispatchBlockedEvent(std::string block_type,
                                   std::string subresource,
                                   int frame_tree_node_id);
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
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;

  // Invoked if an IPC message is coming from a specific RenderFrameHost.
  bool OnMessageReceived(const IPC::Message& message,
      content::RenderFrameHost* render_frame_host) override;
  void OnJavaScriptBlockedWithDetail(
      content::RenderFrameHost* render_frame_host,
      const std::u16string& details);
  void OnFingerprintingBlockedWithDetail(
      content::RenderFrameHost* render_frame_host,
      const std::u16string& details);

  // TODO(iefremov): Refactor this away or at least put into base::NoDestructor.
  // Protects global maps below from being concurrently written on the UI thread
  // and read on the IO thread.
  static base::Lock frame_data_map_lock_;
  static std::map<int, GURL> frame_tree_node_id_to_tab_url_;

 private:
  friend class content::WebContentsUserData<BraveShieldsWebContentsObserver>;
  std::vector<std::string> allowed_script_origins_;
  // We keep a set of the current page's blocked URLs in case the page
  // continually tries to load the same blocked URLs.
  std::set<std::string> blocked_url_paths_;

  WEB_CONTENTS_USER_DATA_KEY_DECL();
  DISALLOW_COPY_AND_ASSIGN(BraveShieldsWebContentsObserver);
};

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_BRAVE_SHIELDS_WEB_CONTENTS_OBSERVER_H_
