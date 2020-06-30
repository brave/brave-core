/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_RENDERER_BRAVE_CONTENT_SETTINGS_AGENT_IMPL_H_
#define BRAVE_RENDERER_BRAVE_CONTENT_SETTINGS_AGENT_IMPL_H_

#include <string>
#include <vector>

#include "base/strings/string16.h"
#include "brave/third_party/blink/renderer/brave_farbling_constants.h"
#include "chrome/renderer/content_settings_agent_impl.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_types.h"

namespace blink {
class WebLocalFrame;
}

// Handles blocking content per content settings for each RenderFrame.
class BraveContentSettingsAgentImpl
    : public ContentSettingsAgentImpl {
 public:
  BraveContentSettingsAgentImpl(content::RenderFrame* render_frame,
      bool should_whitelist,
      service_manager::BinderRegistry* registry);
  ~BraveContentSettingsAgentImpl() override;

 protected:
  bool AllowScript(bool enabled_per_settings) override;
  void DidNotAllowScript() override;
  bool AllowScriptFromSource(bool enabled_per_settings,
      const blink::WebURL& script_url) override;

  bool AllowFingerprinting(bool enabled_per_settings) override;

  BraveFarblingLevel GetBraveFarblingLevel() override;

  bool AllowAutoplay(bool default_value) override;

  void BraveSpecificDidBlockJavaScript(
    const base::string16& details);

  void DidBlockFingerprinting(
    const base::string16& details);

 private:
  bool IsBraveShieldsDown(
      const blink::WebFrame* frame,
      const GURL& secondary_url);

  // RenderFrameObserver
  bool OnMessageReceived(const IPC::Message& message) override;
  void OnAllowScriptsOnce(const std::vector<std::string>& origins);
  void DidCommitProvisionalLoad(bool is_same_document_navigation,
                                ui::PageTransition transition) override;

  bool IsScriptTemporilyAllowed(const GURL& script_url);

  // Origins of scripts which are temporary allowed for this frame in the
  // current load
  base::flat_set<std::string> temporarily_allowed_scripts_;

  // cache blocked script url which will later be used in `DidNotAllowScript()`
  GURL blocked_script_url_;

  // temporary allowed script origins we preloaded for the next load
  base::flat_set<std::string> preloaded_temporarily_allowed_scripts_;

  DISALLOW_COPY_AND_ASSIGN(BraveContentSettingsAgentImpl);
};

#endif  // BRAVE_RENDERER_BRAVE_CONTENT_SETTINGS_AGENT_IMPL_H_
