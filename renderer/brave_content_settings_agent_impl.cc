/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/renderer/brave_content_settings_agent_impl.h"

#include <string>
#include <utility>
#include <vector>

#include "base/bind_helpers.h"
#include "base/feature_list.h"
#include "base/stl_util.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/common/render_messages.h"
#include "brave/common/shield_exceptions.h"
#include "brave/components/brave_shields/common/brave_shield_utils.h"
#include "brave/components/brave_shields/common/features.h"
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
#include "third_party/blink/public/web/web_frame.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "url/url_constants.h"

namespace {

GURL GetOriginOrURL(
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

bool IsBraveShieldsDown(const blink::WebFrame* frame,
                        const GURL& secondary_url,
                        const ContentSettingsForOneType& rules) {
  ContentSetting setting = CONTENT_SETTING_DEFAULT;
  const GURL& primary_url = GetOriginOrURL(frame);

  for (const auto& rule : rules) {
    if (rule.primary_pattern.Matches(primary_url) &&
        rule.secondary_pattern.Matches(secondary_url)) {
      setting = rule.GetContentSetting();
      break;
    }
  }

  return setting == CONTENT_SETTING_BLOCK;
}

}  // namespace

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

bool BraveContentSettingsAgentImpl::IsBraveShieldsDown(
    const blink::WebFrame* frame,
    const GURL& secondary_url) {
  return !content_setting_rules_ ||
         ::IsBraveShieldsDown(frame, secondary_url,
                              content_setting_rules_->brave_shields_rules);
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

  return GetBraveFarblingLevel() != BraveFarblingLevel::MAXIMUM;
}

BraveFarblingLevel BraveContentSettingsAgentImpl::GetBraveFarblingLevel() {
  blink::WebLocalFrame* frame = render_frame()->GetWebFrame();

  ContentSetting setting = CONTENT_SETTING_DEFAULT;
  if (content_setting_rules_) {
    if (IsBraveShieldsDown(
            frame,
            url::Origin(frame->GetDocument().GetSecurityOrigin()).GetURL())) {
      setting = CONTENT_SETTING_ALLOW;
    } else {
      setting = GetBraveFPContentSettingFromRules(
          content_setting_rules_->fingerprinting_rules,
          GetOriginOrURL(frame));
    }
  }

  if (setting == CONTENT_SETTING_BLOCK) {
    VLOG(1) << "farbling level MAXIMUM";
    return BraveFarblingLevel::MAXIMUM;
  } else if (setting == CONTENT_SETTING_ALLOW) {
    VLOG(1) << "farbling level OFF";
    return BraveFarblingLevel::OFF;
  } else {
    VLOG(1) << "farbling level BALANCED";
    return BraveFarblingLevel::BALANCED;
  }
}

bool BraveContentSettingsAgentImpl::AllowAutoplay(bool default_value) {
  blink::WebLocalFrame* frame = render_frame()->GetWebFrame();
  auto origin = frame->GetDocument().GetSecurityOrigin();
  // default allow local files
  if (origin.IsNull() || origin.Protocol().Ascii() == url::kFileScheme) {
    VLOG(1) << "AllowAutoplay=true because no origin or file scheme";
    return true;
  }

  // respect user's site blocklist, if any
  bool ask = false;
  const GURL& primary_url = GetOriginOrURL(frame);
  const GURL& secondary_url =
      url::Origin(frame->GetDocument().GetSecurityOrigin()).GetURL();
  for (const auto& rule : content_setting_rules_->autoplay_rules) {
    if (rule.primary_pattern == ContentSettingsPattern::Wildcard())
        continue;
    if (rule.primary_pattern.Matches(primary_url) &&
        (rule.secondary_pattern == ContentSettingsPattern::Wildcard() ||
         rule.secondary_pattern.Matches(secondary_url))) {
      if (rule.GetContentSetting() == CONTENT_SETTING_BLOCK) {
        VLOG(1) << "AllowAutoplay=false because rule=CONTENT_SETTING_BLOCK";
        return false;
      } else if (rule.GetContentSetting() == CONTENT_SETTING_ASK) {
        VLOG(1) << "AllowAutoplay=ask because rule=CONTENT_SETTING_ASK";
        ask = true;
      }
    }
  }

  if (ask) {
    mojo::Remote<blink::mojom::PermissionService> permission_service;

    render_frame()->GetBrowserInterfaceBroker()->GetInterface(
        permission_service.BindNewPipeAndPassReceiver());

    if (permission_service.get()) {
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
    return false;
  }

  bool allow = ContentSettingsAgentImpl::AllowAutoplay(default_value);
  if (allow)
    VLOG(1) << "AllowAutoplay=true because "
               "ContentSettingsAgentImpl::AllowAutoplay says so";
  else
    VLOG(1) << "AllowAutoplay=false because "
               "ContentSettingsAgentImpl::AllowAutoplay says so";
  return allow;
}
