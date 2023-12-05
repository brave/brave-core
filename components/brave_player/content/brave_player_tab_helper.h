// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_PLAYER_CONTENT_BRAVE_PLAYER_TAB_HELPER_H_
#define BRAVE_COMPONENTS_BRAVE_PLAYER_CONTENT_BRAVE_PLAYER_TAB_HELPER_H_

#include <memory>
#include <string>

#include "base/component_export.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/script_injector/common/mojom/script_injector.mojom.h"
#include "build/build_config.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "mojo/public/cpp/bindings/associated_remote.h"

namespace brave_player {

class BravePlayerService;

// Used to inject Brave-Viewer related scripts into supported web pages.
class COMPONENT_EXPORT(BRAVE_PLAYER_CONTENT) BravePlayerTabHelper
    : public content::WebContentsObserver,
      public content::WebContentsUserData<BravePlayerTabHelper> {
 public:
  // TODO(sko) Figure out if this doesn't have to be delegated to the `//brave`
  // layer. At the moment, requirements are not finalized so we can't tell what
  // will be needed. If we can handle requirements within
  // `//components/brave_player` try moving AdBlockAdjustment dialog to this
  // layer too.
  class Delegate {
   public:
    virtual ~Delegate() = default;
    virtual void ShowAdBlockAdjustmentSuggestion(
        content::WebContents* contents) = 0;
  };

  static void MaybeCreateForWebContents(std::unique_ptr<Delegate> delegate,
                                        content::WebContents* contents,
                                        const int32_t world_id);
  ~BravePlayerTabHelper() override;
  BravePlayerTabHelper(const BravePlayerTabHelper&) = delete;
  BravePlayerTabHelper& operator=(const BravePlayerTabHelper&) = delete;

 private:
  BravePlayerTabHelper(content::WebContents* contents,
                       std::unique_ptr<Delegate> delegate,
                       const int32_t world_id);
  // Called to insert Brave Player eligibility checks into the page.
  void InsertScriptInPage(
      const content::GlobalRenderFrameHostId& render_frame_host_id,
      const std::string& script,
      content::RenderFrameHost::JavaScriptResultCallback cb);
  // Used to insert a Brave Player eligibility test script into the page. The
  // result is used to determine whether to show the Brave Player dialog in
  // |OnTestScriptResult|.
  void InsertTestScript(
      const content::GlobalRenderFrameHostId& render_frame_host_id,
      const GURL& url,
      std::string test_script);
  void OnTestScriptResult(
      const content::GlobalRenderFrameHostId& render_frame_host_id,
      const GURL& url,
      base::Value value);
  mojo::AssociatedRemote<script_injector::mojom::ScriptInjector>& GetRemote(
      content::RenderFrameHost* rfh);
  friend class content::WebContentsUserData<BravePlayerTabHelper>;

  // content::WebContentsObserver overrides
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;
  void DocumentOnLoadCompletedInPrimaryMainFrame() override;

  std::unique_ptr<Delegate> delegate_;
  const int32_t world_id_;
  raw_ptr<BravePlayerService> brave_player_service_;  // NOT OWNED
  bool should_process_ = false;
  // The remote used to send the script to the renderer.
  mojo::AssociatedRemote<script_injector::mojom::ScriptInjector>
      script_injector_remote_;
  base::WeakPtrFactory<BravePlayerTabHelper> weak_factory_{this};

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace brave_player

#endif  // BRAVE_COMPONENTS_BRAVE_PLAYER_CONTENT_BRAVE_PLAYER_TAB_HELPER_H_
