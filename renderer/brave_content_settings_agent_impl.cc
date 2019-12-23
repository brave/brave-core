/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/renderer/brave_content_settings_agent_impl.h"

#include <string>
#include <utility>
#include <vector>

#include "base/bind_helpers.h"
#include "base/stl_util.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/common/render_messages.h"
#include "brave/common/shield_exceptions.h"
#include "brave/content/common/frame_messages.h"
#include "components/content_settings/core/common/content_settings_pattern.h"
#include "components/content_settings/core/common/content_settings_utils.h"
#include "content/public/renderer/render_frame.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "third_party/blink/public/common/browser_interface_broker_proxy.h"
#include "third_party/blink/public/mojom/permissions/permission.mojom.h"
#include "third_party/blink/public/mojom/permissions/permission.mojom-blink.h"
#include "third_party/blink/public/mojom/permissions/permission.mojom-blink-forward.h"
#include "third_party/blink/public/platform/web_url.h"
#include "third_party/blink/public/web/web_document.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "url/url_constants.h"

BraveContentSettingsAgentImpl::BraveContentSettingsAgentImpl(
    content::RenderFrame* render_frame,
    bool should_whitelist,
    service_manager::BinderRegistry* registry)
    : ContentSettingsAgentImpl(render_frame, should_whitelist, registry) {
}

BraveContentSettingsAgentImpl::~BraveContentSettingsAgentImpl() {
}

bool BraveContentSettingsAgentImpl::OnMessageReceived(
    const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(BraveContentSettingsAgentImpl, message)
    IPC_MESSAGE_HANDLER(BraveFrameMsg_AllowScriptsOnce, OnAllowScriptsOnce)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()

  if (handled) return true;
  return ContentSettingsAgentImpl::OnMessageReceived(message);
}

void BraveContentSettingsAgentImpl::OnAllowScriptsOnce(
    const std::vector<std::string>& origins) {
  preloaded_temporarily_allowed_scripts_ = std::move(origins);
}

void BraveContentSettingsAgentImpl::DidCommitProvisionalLoad(
    bool is_same_document_navigation, ui::PageTransition transition) {
  if (!is_same_document_navigation) {
    temporarily_allowed_scripts_ =
      std::move(preloaded_temporarily_allowed_scripts_);
  }

  ContentSettingsAgentImpl::DidCommitProvisionalLoad(
      is_same_document_navigation, transition);
}

bool BraveContentSettingsAgentImpl::IsScriptTemporilyAllowed(
    const GURL& script_url) {
  // Check if scripts from this origin are temporily allowed or not.
  // Also matches the full script URL to support data URL cases which we use
  // the full URL to allow it.
  return base::Contains(
      temporarily_allowed_scripts_, script_url.GetOrigin().spec()) ||
    base::Contains(temporarily_allowed_scripts_, script_url.spec());
}

void BraveContentSettingsAgentImpl::BraveSpecificDidBlockJavaScript(
    const base::string16& details) {
  Send(new BraveViewHostMsg_JavaScriptBlocked(routing_id(), details));
}

bool BraveContentSettingsAgentImpl::AllowScript(
    bool enabled_per_settings) {
  // clear cached url for other flow like directly calling `DidNotAllowScript`
  // without calling `AllowScriptFromSource` first
  blocked_script_url_ = GURL::EmptyGURL();

  blink::WebLocalFrame* frame = render_frame()->GetWebFrame();
  const GURL secondary_url(
      url::Origin(frame->GetDocument().GetSecurityOrigin()).GetURL());

  bool allow = ContentSettingsAgentImpl::AllowScript(enabled_per_settings);
  allow = allow ||
    IsBraveShieldsDown(frame, secondary_url) ||
    IsScriptTemporilyAllowed(secondary_url);

  return allow;
}

void BraveContentSettingsAgentImpl::DidNotAllowScript() {
  if (!blocked_script_url_.is_empty()) {
    BraveSpecificDidBlockJavaScript(
      base::UTF8ToUTF16(blocked_script_url_.spec()));
    blocked_script_url_ = GURL::EmptyGURL();
  }
  ContentSettingsAgentImpl::DidNotAllowScript();
}

bool BraveContentSettingsAgentImpl::AllowScriptFromSource(
    bool enabled_per_settings,
    const blink::WebURL& script_url) {
  const GURL secondary_url(script_url);

  bool allow = ContentSettingsAgentImpl::AllowScriptFromSource(
      enabled_per_settings, script_url);

  // scripts with whitelisted protocols, such as chrome://extensions should
  // be allowed
  bool should_white_list = IsWhitelistedForContentSettings(
      blink::WebSecurityOrigin::Create(script_url),
      render_frame()->GetWebFrame()->GetDocument().Url());

  allow = allow ||
    should_white_list ||
    IsBraveShieldsDown(render_frame()->GetWebFrame(), secondary_url) ||
    IsScriptTemporilyAllowed(secondary_url);

  if (!allow) {
    blocked_script_url_ = secondary_url;
  }

  return allow;
}

void BraveContentSettingsAgentImpl::DidBlockFingerprinting(
    const base::string16& details) {
  Send(new BraveViewHostMsg_FingerprintingBlocked(routing_id(), details));
}

GURL BraveContentSettingsAgentImpl::GetOriginOrURL(
    const blink::WebFrame* frame) {
  url::Origin top_origin = url::Origin(frame->Top()->GetSecurityOrigin());
  // The |top_origin| is unique ("null") e.g., for file:// URLs. Use the
  // document URL as the primary URL in those cases.
  // TODO(alexmos): This is broken for --site-per-process, since top() can be a
  // WebRemoteFrame which does not have a document(), and the WebRemoteFrame's
  // URL is not replicated.  See https://crbug.com/628759.
  if (top_origin.opaque() && frame->Top()->IsWebLocalFrame())
    return frame->Top()->ToWebLocalFrame()->GetDocument().Url();
  return top_origin.GetURL();
}

ContentSetting BraveContentSettingsAgentImpl::GetFPContentSettingFromRules(
    const ContentSettingsForOneType& rules,
    const blink::WebFrame* frame,
    const GURL& secondary_url) {

  if (rules.size() == 0)
    return CONTENT_SETTING_DEFAULT;

  const GURL& primary_url = GetOriginOrURL(frame);

  for (const auto& rule : rules) {
    ContentSettingsPattern secondary_pattern = rule.secondary_pattern;
    if (rule.secondary_pattern ==
        ContentSettingsPattern::FromString("https://firstParty/*")) {
      secondary_pattern = ContentSettingsPattern::FromString(
          "[*.]" + GetOriginOrURL(frame).HostNoBrackets());
    }

    if (rule.primary_pattern.Matches(primary_url) &&
        (secondary_pattern == ContentSettingsPattern::Wildcard() ||
         secondary_pattern.Matches(secondary_url))) {
      return rule.GetContentSetting();
    }
  }

  // for cases which are third party resources and doesn't match any existing
  // rules, block them by default
  return CONTENT_SETTING_BLOCK;
}

bool BraveContentSettingsAgentImpl::IsBraveShieldsDown(
    const blink::WebFrame* frame,
    const GURL& secondary_url) {
  ContentSetting setting = CONTENT_SETTING_DEFAULT;
  const GURL& primary_url = GetOriginOrURL(frame);

  if (content_setting_rules_) {
    for (const auto& rule : content_setting_rules_->brave_shields_rules) {
      if (rule.primary_pattern.Matches(primary_url) &&
          rule.secondary_pattern.Matches(secondary_url)) {
        setting = rule.GetContentSetting();
        break;
      }
    }
  }

  return setting == CONTENT_SETTING_BLOCK;
}

bool BraveContentSettingsAgentImpl::AllowFingerprinting(
    bool enabled_per_settings) {
  if (!enabled_per_settings)
    return false;
  blink::WebLocalFrame* frame = render_frame()->GetWebFrame();
  const GURL secondary_url(
      url::Origin(frame->GetDocument().GetSecurityOrigin()).GetURL());
  if (IsBraveShieldsDown(frame, secondary_url)) {
    return true;
  }
  const GURL& primary_url = GetOriginOrURL(frame);
  if (brave::IsWhitelistedFingerprintingException(primary_url, secondary_url)) {
    return true;
  }
  ContentSettingsForOneType rules;
  if (content_setting_rules_) {
      rules = content_setting_rules_->fingerprinting_rules;
  }
  ContentSettingPatternSource default_rule = ContentSettingPatternSource(
      ContentSettingsPattern::Wildcard(),
      ContentSettingsPattern::FromString("https://firstParty/*"),
      base::Value::FromUniquePtrValue(
          content_settings::ContentSettingToValue(CONTENT_SETTING_ALLOW)),
      std::string(), false);
  rules.push_back(default_rule);
  ContentSetting setting =
      GetFPContentSettingFromRules(rules, frame, secondary_url);
  rules.pop_back();
  bool allow = setting != CONTENT_SETTING_BLOCK;
  allow = allow || IsWhitelistedForContentSettings();

  if (!allow) {
    DidBlockFingerprinting(base::UTF8ToUTF16(secondary_url.spec()));
  }

  return allow;
}

bool BraveContentSettingsAgentImpl::AllowAutoplay(bool default_value) {
  blink::WebLocalFrame* frame = render_frame()->GetWebFrame();
  auto origin = frame->GetDocument().GetSecurityOrigin();
  // default allow local files
  if (origin.IsNull() || origin.Protocol().Ascii() == url::kFileScheme)
    return true;

  bool allow = ContentSettingsAgentImpl::AllowAutoplay(default_value);
  if (allow)
    return true;

  // respect user's site blocklist, if any
  const GURL& primary_url = GetOriginOrURL(frame);
  const GURL& secondary_url =
      url::Origin(frame->GetDocument().GetSecurityOrigin()).GetURL();
  for (const auto& rule : content_setting_rules_->autoplay_rules) {
    if (rule.primary_pattern == ContentSettingsPattern::Wildcard())
        continue;
    if (rule.primary_pattern.Matches(primary_url) &&
        (rule.secondary_pattern == ContentSettingsPattern::Wildcard() ||
         rule.secondary_pattern.Matches(secondary_url))) {
      if (rule.GetContentSetting() == CONTENT_SETTING_BLOCK)
        return false;
    }
  }

  mojo::Remote<blink::mojom::PermissionService> permission_service;

  render_frame()->GetBrowserInterfaceBroker()
    ->GetInterface(permission_service.BindNewPipeAndPassReceiver());

  if (permission_service.get()) {
    // Check (synchronously) whether we already have permission to autoplay.
    // This may call the autoplay whitelist service in the UI thread, which
    // we need to wait for.
    auto has_permission_descriptor =
        blink::mojom::PermissionDescriptor::New();
    has_permission_descriptor->name =
        blink::mojom::PermissionName::AUTOPLAY;
    blink::mojom::blink::PermissionStatus status;
    if (permission_service->HasPermission(
            std::move(has_permission_descriptor), &status)) {
      allow = status == blink::mojom::PermissionStatus::GRANTED;
      if (!allow) {
        // Request permission (asynchronously) but exit this function without
        // allowing autoplay. Depending on settings and previous user choices,
        // this may display visible permissions UI, or an "autoplay blocked"
        // message, or nothing. In any case, we can't wait for it now.
        auto request_permission_descriptor =
            blink::mojom::PermissionDescriptor::New();
        request_permission_descriptor->name =
            blink::mojom::PermissionName::AUTOPLAY;
        permission_service->RequestPermission(
            std::move(request_permission_descriptor), true, base::DoNothing());
      }
    }
  }

  return allow;
}
