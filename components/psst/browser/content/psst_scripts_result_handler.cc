/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/psst/browser/content/psst_scripts_result_handler.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "base/functional/callback_helpers.h"
#include "base/json/json_writer.h"
#include "base/strings/utf_string_conversions.h"
#include "content/public/browser/web_contents.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"

namespace psst {

namespace {
std::string GetScriptWithParams(const std::string& script,
                                std::optional<base::Value> params) {
  if (!params) {
    return script;
  }

  const auto* params_dict = params->GetIfDict();
  if (!params_dict) {
    return script;
  }

  std::optional<std::string> params_json = base::WriteJsonWithOptions(
      *params_dict, base::JSONWriter::OPTIONS_PRETTY_PRINT);
  if (!params_json) {
    return script;
  }

  std::string result =
      base::StrCat({"const params = ", *params_json, ";\n", script});
  return result;
}

}  // namespace

PsstScriptsHandler::PsstScriptsHandler() = default;
PsstScriptsHandler::~PsstScriptsHandler() = default;

PsstScriptsHandlerImpl::PsstScriptsHandlerImpl(
    std::unique_ptr<PsstDialogDelegate> delegate,
    PrefService* prefs,
    content::WebContents* web_contents,
    const content::RenderFrameHost* render_frame_host,
    const int32_t world_id)
    : delegate_(std::move(delegate)),
      prefs_(prefs),
      render_frame_host_id_(render_frame_host->GetGlobalId()),
      web_contents_(web_contents),
      world_id_(world_id) {
  CHECK(world_id_ > content::ISOLATED_WORLD_ID_CONTENT_END);
  CHECK(render_frame_host);
  CHECK(web_contents_);
  CHECK(prefs_);
}

PsstScriptsHandlerImpl::~PsstScriptsHandlerImpl() = default;

void PsstScriptsHandlerImpl::Start() {
  // Here must be implemented the logic of starting the script handler
  // and showing the dialog.
}

PsstDialogDelegate* PsstScriptsHandlerImpl::GetPsstDialogDelegate() {
  return delegate_.get();
}

void PsstScriptsHandlerImpl::InsertUserScript(
    const std::optional<MatchedRule>& rule) {
  if (!rule) {
    return;
  }

  InsertScriptInPage(rule->UserScript(), std::nullopt /* no params */,
                     base::BindOnce(&PsstScriptsHandlerImpl::OnUserScriptResult,
                                    weak_factory_.GetWeakPtr(), rule.value()));
}

void PsstScriptsHandlerImpl::OnUserScriptResult(const MatchedRule& rule,
                                                base::Value script_result) {
  if (!script_result.is_dict()) {
    return;
  }

  InsertScriptInPage(rule.PolicyScript(), std::nullopt, base::DoNothing());
}

void PsstScriptsHandlerImpl::InsertScriptInPage(
    const std::string& script,
    std::optional<base::Value> value,
    InsertScriptInPageCallback cb) {
  content::RenderFrameHost* render_frame_host =
      content::RenderFrameHost::FromID(render_frame_host_id_);

  // Check if render_frame_host is still valid and if starting rfh is the same.
  if (!render_frame_host ||
      render_frame_host_id_ !=
          web_contents_->GetPrimaryMainFrame()->GetGlobalId()) {
    std::move(cb).Run(base::Value(base::Value::Type::NONE));
    return;
  }

  // Add params as JS preamble to the script.
  std::string script_with_params =
      GetScriptWithParams(script, std::move(value));

  GetRemote(render_frame_host)
      ->RequestAsyncExecuteScript(
          world_id_, base::UTF8ToUTF16(script_with_params),
          blink::mojom::UserActivationOption::kDoNotActivate,
          blink::mojom::PromiseResultOption::kAwait, std::move(cb));
}

mojo::AssociatedRemote<script_injector::mojom::ScriptInjector>&
PsstScriptsHandlerImpl::GetRemote(content::RenderFrameHost* rfh) {
  if (!script_injector_remote_.is_bound()) {
    rfh->GetRemoteAssociatedInterfaces()->GetInterface(
        &script_injector_remote_);
  }
  return script_injector_remote_;
}

}  // namespace psst
