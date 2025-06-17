/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PSST_BROWSER_CONTENT_PSST_SCRIPTS_HANDLER_IMPL_H_
#define BRAVE_COMPONENTS_PSST_BROWSER_CONTENT_PSST_SCRIPTS_HANDLER_IMPL_H_

#include <memory>
#include <string>

#include "base/memory/weak_ptr.h"
#include "brave/components/psst/browser/content/consent_dialog_delegate.h"
#include "brave/components/psst/browser/content/psst_scripts_handler.h"
#include "brave/components/psst/browser/core/matched_rule.h"
#include "brave/components/script_injector/common/mojom/script_injector.mojom.h"
#include "content/public/browser/global_routing_id.h"
#include "mojo/public/cpp/bindings/associated_remote.h"

namespace content {
class RenderFrameHost;
}  // namespace content

namespace psst {

class MatchedRule;

class PsstScriptsHandlerImpl : public PsstScriptsHandler {
 public:
  using InsertScriptInPageCallback = base::OnceCallback<void(::base::Value)>;

  explicit PsstScriptsHandlerImpl(
      PrefService* prefs,
      content::WebContents* web_contents,
      const content::RenderFrameHost* render_frame_host,
      std::unique_ptr<ConsentDialogDelegate> consent_dialog_delegate,
      const int32_t world_id);
  ~PsstScriptsHandlerImpl() override;

  void Start() override;

 private:
  void InsertUserScript(std::unique_ptr<MatchedRule> rule);

  void OnUserScriptResult(std::unique_ptr<MatchedRule> rule,
                          base::Value script_result);

  void InsertScriptInPage(const std::string& script,
                          std::optional<base::Value> value,
                          InsertScriptInPageCallback cb);

  const raw_ptr<PrefService> prefs_;

  const content::GlobalRenderFrameHostId render_frame_host_id_;
  base::WeakPtr<content::WebContents> web_contents_;
  const int32_t world_id_;
  std::unique_ptr<ConsentDialogDelegate> consent_dialog_delegate_;

  mojo::AssociatedRemote<script_injector::mojom::ScriptInjector>
      script_injector_remote_;
  base::WeakPtrFactory<PsstScriptsHandlerImpl> weak_factory_{this};
};

}  // namespace psst

#endif  // BRAVE_COMPONENTS_PSST_BROWSER_CONTENT_PSST_SCRIPTS_HANDLER_IMPL_H_
