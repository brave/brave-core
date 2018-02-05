/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_H_

#include "base/macros.h"
#include "base/strings/string16.h"
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
  BraveShieldsWebContentsObserver(content::WebContents*);
  ~BraveShieldsWebContentsObserver() override;

  static void RegisterProfilePrefs(PrefRegistrySimple* registry);
  static void DispatchBlockedEventForWebContents(
      const std::string& block_type,
      const std::string& subresource,
      content::WebContents* web_contents);
  static void DispatchBlockedEvent(
      const std::string& block_type,
      const std::string& subresource,
      int render_process_id,
      int render_frame_id, int frame_tree_node_id);

 protected:
  // content::WebContentsObserver overrides.
  void RenderFrameCreated(content::RenderFrameHost* host) override;

  // Invoked if an IPC message is coming from a specific RenderFrameHost.
  bool OnMessageReceived(const IPC::Message& message,
      content::RenderFrameHost* render_frame_host) override;
  void OnJavaScriptBlockedWithDetail(
      content::RenderFrameHost* render_frame_host,
      const base::string16& details);

  DISALLOW_COPY_AND_ASSIGN(BraveShieldsWebContentsObserver);
};

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_H_
