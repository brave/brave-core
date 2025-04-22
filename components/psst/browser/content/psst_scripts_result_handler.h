/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PSST_BROWSER_CORE_PSST_SCRIPTS_RESULT_HANDLER_H_
#define BRAVE_COMPONENTS_PSST_BROWSER_CORE_PSST_SCRIPTS_RESULT_HANDLER_H_

#include <memory>

#include "base/component_export.h"
#include "base/functional/callback_forward.h"
#include "base/memory/raw_ptr.h"
#include "brave/components/psst/browser/core/matched_rule.h"
#include "brave/components/psst/browser/core/psst_dialog_delegate.h"
#include "brave/components/psst/common/psst_prefs.h"
#include "brave/components/script_injector/common/mojom/script_injector.mojom.h"
#include "components/prefs/pref_service.h"
#include "mojo/public/cpp/bindings/associated_remote.h"

namespace psst {

class PsstOperationContext;

class COMPONENT_EXPORT(PSST_BROWSER_CONTENT) PsstScriptsHandler {
 public:
 using InsertScriptInPageCallback = base::OnceCallback<void (::base::Value)>;
// class PsstScriptsDelegate {
// public:
// PsstScriptsDelegate() = default;
// virtual ~PsstScriptsDelegate() = default;

// using InsertScriptInPageCallback = base::OnceCallback<void (::base::Value)>;

// virtual void InsertScriptInPage(const std::string& script,
//   std::optional<base::Value> value, InsertScriptInPageCallback cb) = 0;
// };

  // using InsertScriptInPageCallback = base::RepeatingCallback<void(
  //   const content::GlobalRenderFrameHostId& render_frame_host_id,
  //   const int32_t& world_id,
  // )>;

  PsstScriptsHandler(const PsstScriptsHandler&) = delete;
  PsstScriptsHandler& operator=(const PsstScriptsHandler&) = delete;

  virtual ~PsstScriptsHandler() = default;

  virtual void Start() = 0;

  virtual PsstDialogDelegate* GetPsstDialogDelegate() = 0;
 protected:
  PsstScriptsHandler() = default;

 private:
  virtual void InsertUserScript(const std::optional<MatchedRule>& rule) = 0;
  virtual void InsertPolicyScript(const std::optional<MatchedRule>& rule) = 0;

  virtual void OnUserScriptResult(const MatchedRule& rule, base::Value script_result) = 0;
  virtual void OnUserDialogAction(
    const bool is_initial,
    const std::string& user_id,
    const MatchedRule& rule,
    std::optional<base::Value> script_params,
    const PsstConsentStatus status,
    const std::vector<std::string>& disabled_checks) = 0;
  virtual void OnPolicyScriptResult(
      const MatchedRule& rule,
      base::Value value) = 0;

  virtual bool TryToLoadContext(const MatchedRule& rule,
                                base::Value& script_result) = 0;
  virtual void ResetContext() = 0;
  virtual void DisablePsst() = 0;

  virtual void InsertScriptInPage(const std::string& script,
    std::optional<base::Value> value, InsertScriptInPageCallback cb) = 0;
   virtual mojo::AssociatedRemote<script_injector::mojom::ScriptInjector>& GetRemote(
         content::RenderFrameHost* rfh) = 0;

};

class COMPONENT_EXPORT(PSST_BROWSER_CONTENT) PsstScriptsHandlerImpl
    : public PsstScriptsHandler {
 public:
  explicit PsstScriptsHandlerImpl(std::unique_ptr<PsstDialogDelegate> delegate,
                                  PrefService* prefs, content::WebContents* web_contents,
                                  content::RenderFrameHost* const render_frame_host);
  ~PsstScriptsHandlerImpl() override;

  void Start() override;

  PsstDialogDelegate* GetPsstDialogDelegate() override;

 private:
  friend class PsstScriptsHandlerUnitTest;

  void InsertUserScript(const std::optional<MatchedRule>& rule) override;
  void InsertPolicyScript(const std::optional<MatchedRule>& rule) override;

  void OnUserScriptResult(const MatchedRule& rule, base::Value script_result) override;
  void OnUserDialogAction(
    const bool is_initial,
    const std::string& user_id,
    const MatchedRule& rule,
    std::optional<base::Value> script_params,
    const PsstConsentStatus status,
    const std::vector<std::string>& disabled_checks) override;
  void OnPolicyScriptResult(
      const MatchedRule& rule,
      base::Value value) override;

  bool TryToLoadContext(const MatchedRule& rule,
                        base::Value& script_result) override;
  void ResetContext() override;
  void DisablePsst() override;

  void InsertScriptInPage(const std::string& script,
    std::optional<base::Value> value, InsertScriptInPageCallback cb) override;

//   Get the remote used to send the script to the renderer.
  mojo::AssociatedRemote<script_injector::mojom::ScriptInjector>& GetRemote(
        content::RenderFrameHost* rfh) override;

  std::unique_ptr<PsstOperationContext> context_;
  std::unique_ptr<PsstDialogDelegate> delegate_;
  const raw_ptr<PrefService> prefs_;
//  std::unique_ptr<PsstScriptsDelegate> script_delegate_;

const content::GlobalRenderFrameHostId render_frame_host_id_;
raw_ptr<content::WebContents> web_contents_{nullptr};
const int32_t world_id_;

mojo::AssociatedRemote<script_injector::mojom::ScriptInjector> script_injector_remote_;

  base::WeakPtrFactory<PsstScriptsHandlerImpl> weak_factory_{this};
};

}  // namespace psst

#endif  // BRAVE_COMPONENTS_PSST_BROWSER_CORE_PSST_SCRIPTS_RESULT_HANDLER_H_
