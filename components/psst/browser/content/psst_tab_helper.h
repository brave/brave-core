// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_PSST_BROWSER_CONTENT_PSST_TAB_HELPER_H_
#define BRAVE_COMPONENTS_PSST_BROWSER_CONTENT_PSST_TAB_HELPER_H_

#include <cstdint>
#include <memory>

#include "base/component_export.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/psst/browser/content/psst_scripts_result_handler.h"
#include "brave/components/psst/browser/core/psst_dialog_delegate.h"
#include "brave/components/psst/browser/core/psst_opeartion_context.h"
#include "brave/components/script_injector/common/mojom/script_injector.mojom.h"
#include "build/build_config.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "chrome/browser/ui/webui/constrained_web_dialog_ui.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

namespace psst {

class PsstRuleRegistry;

// Used to inject PSST scripts into the page, based on PSST rules.
class COMPONENT_EXPORT(PSST_BROWSER_CONTENT) PsstTabHelper
    : public content::WebContentsObserver {
 public:
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

  void OnDisablePsst();

  void ResetContext();

  // Get the remote used to send the script to the renderer.
  //   mojo::AssociatedRemote<script_injector::mojom::ScriptInjector>&
  //   GetRemote(
  //       content::RenderFrameHost* rfh);

  friend class content::WebContentsUserData<PsstTabHelper>;

  // content::WebContentsObserver overrides
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;
  void DocumentOnLoadCompletedInPrimaryMainFrame() override;

  std::unique_ptr<PsstScriptsHandler> script_handler_;
  const raw_ptr<PrefService> prefs_;
  const raw_ptr<PsstRuleRegistry> psst_rule_registry_;
  bool should_process_{false};

  base::WeakPtrFactory<PsstTabHelper> weak_factory_{this};

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace psst

#endif  // BRAVE_COMPONENTS_PSST_BROWSER_CONTENT_PSST_TAB_HELPER_H_
