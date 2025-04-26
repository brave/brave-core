// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_PSST_BROWSER_CONTENT_PSST_TAB_HELPER_H_
#define BRAVE_COMPONENTS_PSST_BROWSER_CONTENT_PSST_TAB_HELPER_H_

#include <memory>
#include <string>

#include "base/component_export.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "brave/components/psst/browser/core/matched_rule.h"
#include "brave/components/psst/common/psst_prefs.h"
#include "brave/components/script_injector/common/mojom/script_injector.mojom.h"
#include "build/build_config.h"
#include "components/prefs/pref_service.h"
#include "components/sessions/core/session_id.h"
#include "content/public/browser/media_player_id.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "mojo/public/cpp/bindings/associated_remote.h"

namespace psst {

class PsstRuleRegistry;

// Used to inject PSST scripts into the page, based on PSST rules.
class COMPONENT_EXPORT(PSST_BROWSER_CONTENT) PsstTabHelper
    : public content::WebContentsObserver,
      public content::WebContentsUserData<PsstTabHelper> {
 public:
  class Delegate {
   public:
    virtual ~Delegate() = default;
    virtual void ShowPsstConsentDialog(content::WebContents* contents,
                                       bool prompt_for_new_version,
                                       const std::string& list_of_changes,
                                       base::OnceClosure yes_cb,
                                       base::OnceClosure no_cb) = 0;
  };

  static void MaybeCreateForWebContents(content::WebContents* contents,
                                        std::unique_ptr<Delegate> delegate,
                                        const int32_t world_id);
  ~PsstTabHelper() override;
  PsstTabHelper(const PsstTabHelper&) = delete;
  PsstTabHelper& operator=(const PsstTabHelper&) = delete;

 private:
  PsstTabHelper(content::WebContents*,
                std::unique_ptr<Delegate> delegate,
                const int32_t world_id);

  // Insert scripts for the rfh using the script_injector mojo interface.
  void InsertScriptInPage(
      const content::GlobalRenderFrameHostId& render_frame_host_id,
      const std::string& script,
      std::optional<base::Value> value,
      content::RenderFrameHost::JavaScriptResultCallback cb);

  // Script injection methods. The flow is:
  // 1. insert get user script and get logged in user id.
  // 2. if valid and we need to, get consent and/or insert test script.
  // 3. if we can make changes, insert policy script to make changes.
  void InsertUserScript(
      const content::GlobalRenderFrameHostId& render_frame_host_id,
      const MatchedRule& rule);
  void OnUserScriptResult(
      const MatchedRule& rule,
      const content::GlobalRenderFrameHostId& render_frame_host_id,
      base::Value value);
  void OnUserDialogAction(
      const std::string& user_id,
      const MatchedRule& rule,
      std::optional<base::Value> params,
      const content::GlobalRenderFrameHostId& render_frame_host_id,
      PsstConsentStatus status);
  void OnTestScriptResult(
      const std::string& user_id,
      const MatchedRule& rule,
      const content::GlobalRenderFrameHostId& render_frame_host_id,
      base::Value value);

  // Get the remote used to send the script to the renderer.
  mojo::AssociatedRemote<script_injector::mojom::ScriptInjector>& GetRemote(
      content::RenderFrameHost* rfh);

  friend class content::WebContentsUserData<PsstTabHelper>;

  // content::WebContentsObserver overrides
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;
  void DocumentOnLoadCompletedInPrimaryMainFrame() override;
  std::unique_ptr<Delegate> delegate_;

  const int32_t world_id_;
  const raw_ptr<PrefService> prefs_;
  const raw_ptr<PsstRuleRegistry> psst_rule_registry_;  // NOT OWNED
  bool should_process_ = false;
  // The remote used to send the script to the renderer.
  mojo::AssociatedRemote<script_injector::mojom::ScriptInjector>
      script_injector_remote_;
  base::WeakPtrFactory<PsstTabHelper> weak_factory_{this};

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace psst

#endif  // BRAVE_COMPONENTS_PSST_BROWSER_CONTENT_PSST_TAB_HELPER_H_
