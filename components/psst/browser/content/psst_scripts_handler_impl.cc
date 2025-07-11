// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/psst/browser/content/psst_scripts_handler_impl.h"

#include <memory>
#include <string>
#include <utility>

#include "base/functional/callback_helpers.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/psst/browser/core/matched_rule.h"
#include "brave/components/psst/browser/core/psst_rule_registry.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"

namespace psst {

PsstScriptsHandlerImpl::PsstScriptsHandlerImpl(
    PrefService* prefs,
    content::WebContents* web_contents,
    const content::RenderFrameHost* render_frame_host,
    const int32_t world_id)
    : prefs_(prefs),
      render_frame_host_id_(render_frame_host->GetGlobalId()),
      web_contents_(web_contents->GetWeakPtr()),
      world_id_(world_id) {
  CHECK(world_id_ > content::ISOLATED_WORLD_ID_CONTENT_END);
  CHECK(render_frame_host_id_);
  CHECK(web_contents_);
  CHECK(prefs_);
}

PsstScriptsHandlerImpl::~PsstScriptsHandlerImpl() = default;

void PsstScriptsHandlerImpl::Start() {
  CHECK(web_contents_);
  auto url = web_contents_->GetLastCommittedURL();

  PsstRuleRegistry::GetInstance()->CheckIfMatch(
      url, base::BindOnce(&PsstScriptsHandlerImpl::InsertUserScript,
                          weak_factory_.GetWeakPtr()));
}

void PsstScriptsHandlerImpl::InsertUserScript(
    std::unique_ptr<MatchedRule> rule) {
  if (!rule) {
    return;
  }

  const auto user_script = rule->user_script();
  InsertScriptInPage(
      user_script, base::BindOnce(&PsstScriptsHandlerImpl::OnUserScriptResult,
                                  weak_factory_.GetWeakPtr(), std::move(rule)));
}

void PsstScriptsHandlerImpl::OnUserScriptResult(
    std::unique_ptr<MatchedRule> rule,
    base::Value script_result) {
  if (rule && !script_result.is_dict()) {
    return;
  }

  InsertScriptInPage(rule->policy_script(), base::DoNothing());
}

void PsstScriptsHandlerImpl::InsertScriptInPage(const std::string& script,
                                                InsertScriptInPageCallback cb) {
  content::RenderFrameHost* render_frame_host =
      content::RenderFrameHost::FromID(render_frame_host_id_);

  // Check if render_frame_host is still valid and if starting rfh is the same.
  if (!render_frame_host || !web_contents_ ||
      render_frame_host_id_ !=
          web_contents_->GetPrimaryMainFrame()->GetGlobalId()) {
    std::move(cb).Run(base::Value(base::Value::Type::NONE));
    return;
  }

  render_frame_host->ExecuteJavaScriptInIsolatedWorld(base::UTF8ToUTF16(script),
                                                      std::move(cb), world_id_);
}

}  // namespace psst
