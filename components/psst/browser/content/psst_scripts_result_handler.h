/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PSST_BROWSER_CONTENT_PSST_SCRIPTS_RESULT_HANDLER_H_
#define BRAVE_COMPONENTS_PSST_BROWSER_CONTENT_PSST_SCRIPTS_RESULT_HANDLER_H_

#include <memory>
#include <string>

#include "base/component_export.h"
#include "brave/components/script_injector/common/mojom/script_injector.mojom.h"
#include "content/public/browser/web_contents.h"
#include "mojo/public/cpp/bindings/associated_remote.h"

class PrefService;

namespace psst {

class MatchedRule;

class COMPONENT_EXPORT(PSST_BROWSER_CONTENT) PsstScriptsHandler {
 public:
  using InsertScriptInPageCallback = base::OnceCallback<void(::base::Value)>;

  PsstScriptsHandler(const PsstScriptsHandler&) = delete;
  PsstScriptsHandler& operator=(const PsstScriptsHandler&) = delete;

  PsstScriptsHandler();
  virtual ~PsstScriptsHandler();

  virtual void Start() = 0;

 private:
  virtual void InsertUserScript(const std::optional<MatchedRule>& rule) = 0;

  virtual void OnUserScriptResult(const MatchedRule& rule,
                                  base::Value script_result) = 0;

  virtual void InsertScriptInPage(const std::string& script,
                                  std::optional<base::Value> value,
                                  InsertScriptInPageCallback cb) = 0;

  virtual mojo::AssociatedRemote<script_injector::mojom::ScriptInjector>&
  GetRemote(content::RenderFrameHost* rfh) = 0;
};

class COMPONENT_EXPORT(PSST_BROWSER_CONTENT) PsstScriptsHandlerImpl
    : public PsstScriptsHandler {
 public:
  explicit PsstScriptsHandlerImpl(
      PrefService* prefs,
      content::WebContents* web_contents,
      const content::RenderFrameHost* render_frame_host,
      const int32_t world_id);
  ~PsstScriptsHandlerImpl() override;

  void Start() override;

 private:
  void InsertUserScript(const std::optional<MatchedRule>& rule) override;

  void OnUserScriptResult(const MatchedRule& rule,
                          base::Value script_result) override;

  void InsertScriptInPage(const std::string& script,
                          std::optional<base::Value> value,
                          InsertScriptInPageCallback cb) override;

  //   Get the remote used to send the script to the renderer.
  mojo::AssociatedRemote<script_injector::mojom::ScriptInjector>& GetRemote(
      content::RenderFrameHost* rfh) override;

  const raw_ptr<PrefService> prefs_;

  const content::GlobalRenderFrameHostId render_frame_host_id_;
  raw_ptr<content::WebContents> web_contents_{nullptr};
  const int32_t world_id_;

  mojo::AssociatedRemote<script_injector::mojom::ScriptInjector>
      script_injector_remote_;
  base::WeakPtrFactory<PsstScriptsHandlerImpl> weak_factory_{this};
};

}  // namespace psst

#endif  // BRAVE_COMPONENTS_PSST_BROWSER_CONTENT_PSST_SCRIPTS_RESULT_HANDLER_H_
