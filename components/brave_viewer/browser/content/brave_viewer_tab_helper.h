
// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_VIEWER_BROWSER_CONTENT_BRAVE_VIEWER_TAB_HELPER_H_
#define BRAVE_COMPONENTS_BRAVE_VIEWER_BROWSER_CONTENT_BRAVE_VIEWER_TAB_HELPER_H_

#include <string>

#include "base/component_export.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/script_injector/common/mojom/script_injector.mojom.h"
#include "build/build_config.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "mojo/public/cpp/bindings/associated_remote.h"

namespace brave_viewer {

class BraveViewerService;

// Used to inject Brave-Viewer related scripts into supported web pages.
class COMPONENT_EXPORT(BRAVE_VIEWER_BROWSER_CONTENT) BraveViewerTabHelper
    : public content::WebContentsObserver,
      public content::WebContentsUserData<BraveViewerTabHelper> {
 public:
  static void MaybeCreateForWebContents(content::WebContents* contents,
                                        const int32_t world_id);
  ~BraveViewerTabHelper() override;
  BraveViewerTabHelper(const BraveViewerTabHelper&) = delete;
  BraveViewerTabHelper& operator=(const BraveViewerTabHelper&) = delete;

 private:
  BraveViewerTabHelper(content::WebContents*, const int32_t world_id);
  // Called to insert Brave Viewer eligibility checks into the page.
  void InsertScriptInPage(
      const content::GlobalRenderFrameHostId& render_frame_host_id,
      const std::string& script,
      content::RenderFrameHost::JavaScriptResultCallback cb);
  // Used to insert a Brave Viewer eligibility test script into the page. The
  // result is used to determine whether to show the Brave Viewer dialog in
  // |OnTestScriptResult|.
  void InsertTestScript(
      const content::GlobalRenderFrameHostId& render_frame_host_id,
      std::string test_script);
  void OnTestScriptResult(
      const content::GlobalRenderFrameHostId& render_frame_host_id,
      base::Value value);
  mojo::AssociatedRemote<script_injector::mojom::ScriptInjector>& GetRemote(
      content::RenderFrameHost* rfh);
  friend class content::WebContentsUserData<BraveViewerTabHelper>;

  // content::WebContentsObserver overrides
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;
  void DocumentOnLoadCompletedInPrimaryMainFrame() override;

  const int32_t world_id_;
  const raw_ptr<BraveViewerService> brave_viewer_service_;  // NOT OWNED
  bool should_process_ = false;
  // The remote used to send the script to the renderer.
  mojo::AssociatedRemote<script_injector::mojom::ScriptInjector>
      script_injector_remote_;
  base::WeakPtrFactory<BraveViewerTabHelper> weak_factory_{this};

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace brave_viewer

#endif  // BRAVE_COMPONENTS_BRAVE_VIEWER_BROWSER_CONTENT_BRAVE_VIEWER_TAB_HELPER_H_
