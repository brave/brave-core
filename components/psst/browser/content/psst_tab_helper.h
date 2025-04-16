// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_PSST_BROWSER_CONTENT_PSST_TAB_HELPER_H_
#define BRAVE_COMPONENTS_PSST_BROWSER_CONTENT_PSST_TAB_HELPER_H_

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/component_export.h"
#include "base/functional/callback_forward.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/values.h"
#include "brave/components/psst/browser/core/matched_rule.h"
#include "brave/components/psst/browser/core/psst_dialog_delegate.h"
#include "brave/components/psst/browser/core/psst_opeartion_context.h"
#include "brave/components/psst/common/psst_prefs.h"
#include "brave/components/script_injector/common/mojom/script_injector.mojom.h"
#include "build/build_config.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "chrome/browser/ui/webui/constrained_web_dialog_ui.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/devtools_agent_host_client.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "mojo/public/cpp/bindings/associated_remote.h"
#include "url/gurl.h"

namespace psst {

class PsstRuleRegistry;

// Used to inject PSST scripts into the page, based on PSST rules.
class COMPONENT_EXPORT(PSST_BROWSER_CONTENT) PsstTabHelper
    : public content::WebContentsObserver {
 public:

  

//   void AddObserver(Delegate::Observer* obs);
//   void RemoveObserver(Delegate::Observer* obs);
//   bool HasObserver(Delegate::Observer* observer);

  static std::unique_ptr<PsstTabHelper> MaybeCreateForWebContents(
      content::WebContents* contents,
      std::unique_ptr<PsstDialogDelegate> delegate);

  ~PsstTabHelper() override;
  PsstTabHelper(const PsstTabHelper&) = delete;
  PsstTabHelper& operator=(const PsstTabHelper&) = delete;

  PsstDialogDelegate* GetPsstDialogDelegate() const;

 private:
  PsstTabHelper(content::WebContents*,
                std::unique_ptr<PsstDialogDelegate> delegate,
                const int32_t world_id);

  // Insert scripts for the rfh using the script_injector mojo interface.
  void InsertScriptInPage(
      const content::GlobalRenderFrameHostId& render_frame_host_id,
      const int32_t& world_id,
      const std::string& script,
      std::optional<base::Value> value,
      content::RenderFrameHost::JavaScriptResultCallback cb);

  void InsertUserScript(
      const content::GlobalRenderFrameHostId& render_frame_host_id,
      const std::optional<MatchedRule>& rule);
  void InsertPolicyScript(
      const content::GlobalRenderFrameHostId& render_frame_host_id,
      const std::optional<MatchedRule>& rule);

  void OnUserScriptResult(
      const MatchedRule& rule,
      const content::GlobalRenderFrameHostId& render_frame_host_id,
      base::Value value);
  void OnUserDialogAction(
      const bool is_initial,
      const std::string& user_id,
      const MatchedRule& rule,
      std::optional<base::Value> params,
      const content::GlobalRenderFrameHostId& render_frame_host_id,
      PsstConsentStatus status,
      const std::vector<std::string>& disabled_checks);
  void OnPolicyScriptResult(
      const std::string& user_id,
      const MatchedRule& rule,
      const content::GlobalRenderFrameHostId& render_frame_host_id,
      base::Value value);

  void OnDisablePsst();

  void ResetContext();

  // Get the remote used to send the script to the renderer.
  mojo::AssociatedRemote<script_injector::mojom::ScriptInjector>& GetRemote(
      content::RenderFrameHost* rfh);

  friend class content::WebContentsUserData<PsstTabHelper>;

  // content::WebContentsObserver overrides
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;
  void DocumentOnLoadCompletedInPrimaryMainFrame() override;

  std::unique_ptr<PsstOperationContext> psst_operation_context_;
  std::unique_ptr<PsstDialogDelegate> delegate_;
  const int32_t world_id_;
  PrefChangeRegistrar pref_change_registrar_;
  const raw_ptr<PrefService> prefs_;
  const raw_ptr<PsstRuleRegistry> psst_rule_registry_;
  bool should_process_{false};
  // The remote used to send the script to the renderer.
  mojo::AssociatedRemote<script_injector::mojom::ScriptInjector>
      script_injector_remote_;
  base::WeakPtrFactory<PsstTabHelper> weak_factory_{this};

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace psst

#endif  // BRAVE_COMPONENTS_PSST_BROWSER_CONTENT_PSST_TAB_HELPER_H_
