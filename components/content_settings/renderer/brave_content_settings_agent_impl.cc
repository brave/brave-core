/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/content_settings/renderer/brave_content_settings_agent_impl.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/contains.h"
#include "base/feature_list.h"
#include "base/functional/callback_helpers.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/brave_shields/core/common/brave_shield_utils.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "components/content_settings/core/common/content_settings_pattern.h"
#include "components/content_settings/core/common/content_settings_utils.h"
#include "content/public/renderer/render_frame.h"
#include "net/base/features.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_registry.h"
#include "third_party/blink/public/platform/web_url.h"
#include "third_party/blink/public/web/web_document.h"
#include "third_party/blink/public/web/web_frame.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "url/origin.h"
#include "url/url_constants.h"

namespace content_settings {
namespace {

constexpr char kJavascriptExtension[] = ".js";

bool IsFrameWithOpaqueOrigin(blink::WebFrame* frame) {
  // Storage access is keyed off the top origin and the frame's origin.
  // It will be denied any opaque origins so have this method to return early
  // instead of making a Sync IPC call.
  return frame->GetSecurityOrigin().IsOpaque() ||
         frame->Top()->GetSecurityOrigin().IsOpaque();
}

GURL GetOriginOrURL(const blink::WebFrame* frame) {
  url::Origin top_origin = url::Origin(frame->Top()->GetSecurityOrigin());
  // The |top_origin| is unique ("null") e.g., for file:// URLs. Use the
  // document URL as the primary URL in those cases.
  // TODO(alexmos): This is broken for --site-per-process, since top() can be a
  // WebRemoteFrame which does not have a document(), and the WebRemoteFrame's
  // URL is not replicated.  See https://crbug.com/628759.
  if (top_origin.opaque() && frame->Top()->IsWebLocalFrame()) {
    return frame->Top()->ToWebLocalFrame()->GetDocument().Url();
  }
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

// Skips everything except main frame domain and javascript urls.
bool ShouldSkipResource(const GURL& resource_url) {
  return (resource_url.path_piece().empty() ||
          ((resource_url.path_piece().size() > 1) &&
           !resource_url.path_piece().ends_with(kJavascriptExtension)));
}

}  // namespace

BraveContentSettingsAgentImpl::BraveContentSettingsAgentImpl(
    content::RenderFrame* render_frame,
    std::unique_ptr<Delegate> delegate)
    : ContentSettingsAgentImpl(render_frame,
                               std::move(delegate)) {
  render_frame->GetAssociatedInterfaceRegistry()
      ->AddInterface<brave_shields::mojom::BraveShields>(base::BindRepeating(
          &BraveContentSettingsAgentImpl::BindBraveShieldsReceiver,
          base::Unretained(this)));
}

BraveContentSettingsAgentImpl::~BraveContentSettingsAgentImpl() = default;

bool BraveContentSettingsAgentImpl::IsScriptTemporarilyAllowed(
    const GURL& script_url) {
  // Check if scripts from this origin are temporarily allowed or not.
  // Also matches the full script URL to support data URL cases which we use
  // the full URL to allow it.
  if (!shields_settings_) {
    return false;
  }
  bool allow = base::Contains(shields_settings_->origins_to_allow_scripts,
                              url::Origin::Create(script_url).Serialize()) ||
               base::Contains(shields_settings_->origins_to_allow_scripts,
                              script_url.spec());
  if (!allow) {
    // Also check rules in the main frame, because this frame rules may be out
    // of sync.
    content::RenderFrame* main_frame = render_frame()->GetMainRenderFrame();
    if (main_frame && main_frame != render_frame()) {
      allow = static_cast<BraveContentSettingsAgentImpl*>(
                  ContentSettingsAgentImpl::Get(main_frame))
                  ->IsScriptTemporarilyAllowed(script_url);
    }
  }
  return allow;
}

bool BraveContentSettingsAgentImpl::IsReduceLanguageEnabled() {
  if (!shields_settings_) {
    return false;
  }
  return shields_settings_->reduce_language;
}

void BraveContentSettingsAgentImpl::BraveSpecificDidAllowJavaScriptOnce(
    const GURL& resource_url) {
  // This will be called for all resources on a page, we want to notify only
  // about frame domain and javascript resources.
  if (ShouldSkipResource(resource_url)) {
    return;
  }
  GetOrCreateBraveShieldsRemote()->OnJavaScriptAllowedOnce(
      base::UTF8ToUTF16(resource_url.spec()));
}

void BraveContentSettingsAgentImpl::BraveSpecificDidBlockJavaScript(
    const std::u16string& details) {
  mojo::AssociatedRemote<brave_shields::mojom::BraveShieldsHost> remote;
  GetOrCreateBraveShieldsRemote()->OnJavaScriptBlocked(details);
}

bool BraveContentSettingsAgentImpl::AllowScript(bool enabled_per_settings) {
  // clear cached url for other flow like directly calling `DidNotAllowScript`
  // without calling `AllowScriptFromSource` first
  blocked_script_url_ = GURL::EmptyGURL();

  blink::WebLocalFrame* frame = render_frame()->GetWebFrame();
  const GURL secondary_url(url::Origin(frame->GetSecurityOrigin()).GetURL());
  bool allow = ContentSettingsAgentImpl::AllowScript(enabled_per_settings);
  auto is_shields_down = IsBraveShieldsDown(frame, secondary_url);
  auto is_script_temporarily_allowed =
      IsScriptTemporarilyAllowed(secondary_url);
  allow = allow || is_shields_down || is_script_temporarily_allowed;
  if (!allow) {
    blocked_script_url_ = secondary_url;
  } else if (!is_shields_down) {
    if (is_script_temporarily_allowed) {
      BraveSpecificDidAllowJavaScriptOnce(secondary_url);
    }
  }
  return allow;
}

void BraveContentSettingsAgentImpl::DidNotAllowScript() {
  if (blocked_script_url_.is_empty()) {
    blink::WebLocalFrame* frame = render_frame()->GetWebFrame();
    blocked_script_url_ = url::Origin(frame->GetSecurityOrigin()).GetURL();
  }
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
  GURL secondary_url(script_url);
  // For scripts w/o sources it should report the domain / site used for
  // executing the frame (which most, but not all, of the time will just be from
  // document.location
  if (secondary_url.SchemeIsLocal()) {
    secondary_url =
        url::Origin(render_frame()->GetWebFrame()->GetSecurityOrigin())
            .GetURL();
  }
  bool allow = ContentSettingsAgentImpl::AllowScriptFromSource(
      enabled_per_settings, script_url);

  auto is_shields_down =
      IsBraveShieldsDown(render_frame()->GetWebFrame(), secondary_url);
  auto is_script_temporarily_allowed =
      IsScriptTemporarilyAllowed(secondary_url);
  allow = allow || is_shields_down || is_script_temporarily_allowed;

  if (!allow) {
    blocked_script_url_ = secondary_url;
  } else if (!is_shields_down) {
    if (is_script_temporarily_allowed) {
      BraveSpecificDidAllowJavaScriptOnce(secondary_url);
    }
  }

  return allow;
}

blink::WebSecurityOrigin
BraveContentSettingsAgentImpl::GetEphemeralStorageOriginSync() {
  if (!base::FeatureList::IsEnabled(net::features::kBraveEphemeralStorage)) {
    return {};
  }

  blink::WebLocalFrame* frame = render_frame()->GetWebFrame();

  if (!frame || IsFrameWithOpaqueOrigin(frame)) {
    return {};
  }

  auto frame_origin = url::Origin(frame->GetSecurityOrigin());
  const auto ephemeral_storage_origin_it =
      cached_ephemeral_storage_origins_.find(frame_origin);
  if (ephemeral_storage_origin_it != cached_ephemeral_storage_origins_.end()) {
    return ephemeral_storage_origin_it->second;
  }

  auto top_origin = url::Origin(frame->Top()->GetSecurityOrigin());
  // If first party ephemeral storage is enabled, we should always ask the
  // browser if a frame should use ephemeral storage or not.
  if (!base::FeatureList::IsEnabled(
          net::features::kBraveFirstPartyEphemeralStorage) &&
      net::registry_controlled_domains::SameDomainOrHost(
          top_origin, frame_origin,
          net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES)) {
    return {};
  }

  std::optional<url::Origin> optional_ephemeral_storage_origin;
  GetContentSettingsManager().AllowEphemeralStorageAccess(
      frame->GetLocalFrameToken(), frame_origin,
      frame->GetDocument().SiteForCookies(), top_origin,
      &optional_ephemeral_storage_origin);
  blink::WebSecurityOrigin ephemeral_storage_origin(
      optional_ephemeral_storage_origin
          ? blink::WebSecurityOrigin(*optional_ephemeral_storage_origin)
          : blink::WebSecurityOrigin());
  cached_ephemeral_storage_origins_[frame_origin] = ephemeral_storage_origin;
  return ephemeral_storage_origin;
}

bool BraveContentSettingsAgentImpl::AllowStorageAccessSync(
    StorageType storage_type) {
  bool result = ContentSettingsAgentImpl::AllowStorageAccessSync(storage_type);
  if (result) {
    return true;
  }

  if (storage_type == StorageType::kLocalStorage ||
      storage_type == StorageType::kSessionStorage) {
    return !GetEphemeralStorageOriginSync().IsNull();
  }

  return false;
}

bool BraveContentSettingsAgentImpl::IsBraveShieldsDown(
    const blink::WebFrame* frame,
    const GURL& secondary_url) {
  return !content_setting_rules_ ||
         ::content_settings::IsBraveShieldsDown(
             frame, secondary_url, content_setting_rules_->brave_shields_rules);
}

bool BraveContentSettingsAgentImpl::IsCosmeticFilteringEnabled(
    const GURL& url) {
  blink::WebLocalFrame* frame = render_frame()->GetWebFrame();
  GURL secondary_url = GURL();

  ContentSetting setting = CONTENT_SETTING_DEFAULT;
  if (content_setting_rules_) {
    const GURL& primary_url = GetOriginOrURL(frame);

    for (const auto& rule : content_setting_rules_->cosmetic_filtering_rules) {
      if (rule.primary_pattern.Matches(primary_url) &&
          rule.secondary_pattern.Matches(secondary_url)) {
        setting = rule.GetContentSetting();
        break;
      }
    }
  }

  return base::FeatureList::IsEnabled(
             brave_shields::features::kBraveAdblockCosmeticFiltering) &&
         !IsBraveShieldsDown(frame, secondary_url) &&
         (setting != CONTENT_SETTING_ALLOW);
}

bool BraveContentSettingsAgentImpl::IsFirstPartyCosmeticFilteringEnabled(
    const GURL& url) {
  blink::WebLocalFrame* frame = render_frame()->GetWebFrame();
  GURL secondary_url = GURL("https://firstParty/");

  ContentSetting setting = CONTENT_SETTING_DEFAULT;
  if (content_setting_rules_) {
    const GURL& primary_url = GetOriginOrURL(frame);

    for (const auto& rule : content_setting_rules_->cosmetic_filtering_rules) {
      if (rule.primary_pattern.Matches(primary_url) &&
          rule.secondary_pattern.Matches(secondary_url)) {
        setting = rule.GetContentSetting();
        break;
      }
    }
  }

  return setting == CONTENT_SETTING_BLOCK;
}

void BraveContentSettingsAgentImpl::DidCommitProvisionalLoad(
    ui::PageTransition transition) {
  ContentSettingsAgentImpl::DidCommitProvisionalLoad(transition);
  // Invalidate Ephemeral Storage opaque origins. Page reload might change the
  // Ephemeral Storage mode, in this case we should re-request it.
  cached_ephemeral_storage_origins_.clear();
}

brave_shields::mojom::ShieldsSettingsPtr
BraveContentSettingsAgentImpl::GetBraveShieldsSettings(
    ContentSettingsType webcompat_settings_type) {
  GetOrCreateBraveShieldsRemote()->OnWebcompatFeatureInvoked(
      webcompat_settings_type);
  blink::WebLocalFrame* frame = render_frame()->GetWebFrame();

  ContentSetting setting = CONTENT_SETTING_DEFAULT;
  if (content_setting_rules_) {
    if (IsBraveShieldsDown(frame,
                           url::Origin(frame->GetSecurityOrigin()).GetURL())) {
      setting = CONTENT_SETTING_ALLOW;
    } else {
      setting = brave_shields::GetBraveFPContentSettingFromRules(
          content_setting_rules_->fingerprinting_rules, GetOriginOrURL(frame));
    }
    if (setting != CONTENT_SETTING_ALLOW) {
      auto webcompat_setting =
          brave_shields::GetBraveWebcompatContentSettingFromRules(
              content_setting_rules_->webcompat_rules, GetOriginOrURL(frame),
              webcompat_settings_type);
      if (webcompat_setting == CONTENT_SETTING_ALLOW) {
        setting = CONTENT_SETTING_ALLOW;
      }
    }
  }

  BraveFarblingLevel farbling_level = BraveFarblingLevel::BALANCED;
  if (setting == CONTENT_SETTING_BLOCK) {
    DVLOG(1) << "farbling level MAXIMUM";
    farbling_level = BraveFarblingLevel::MAXIMUM;
  } else if (setting == CONTENT_SETTING_ALLOW) {
    DVLOG(1) << "farbling level OFF";
    farbling_level = BraveFarblingLevel::OFF;
  } else {
    DVLOG(1) << "farbling level BALANCED";
    farbling_level = BraveFarblingLevel::BALANCED;
  }

  if (shields_settings_) {
    auto shields_settings = shields_settings_.Clone();
    shields_settings->farbling_level = farbling_level;
    return shields_settings;
  } else {
    // TODO(goodov): Parent or Incumbent frame should be used in this case.
    DCHECK(!HasContentSettingsRules());
    return brave_shields::mojom::ShieldsSettings::New(
        farbling_level, std::vector<std::string>(), false);
  }
}

bool BraveContentSettingsAgentImpl::AllowAutoplay(bool play_requested) {
  blink::WebLocalFrame* frame = render_frame()->GetWebFrame();
  auto origin = frame->GetSecurityOrigin();
  // default allow local files
  if (origin.IsNull() || origin.Protocol().Ascii() == url::kFileScheme) {
    DVLOG(1) << "AllowAutoplay=true because no origin or file scheme";
    return true;
  }

  // respect user's site blocklist, if any
  if (content_setting_rules_) {
    ContentSetting setting = GetContentSettingFromRulesImpl(
        content_setting_rules_->autoplay_rules, url::Origin(origin).GetURL());
    if (setting == CONTENT_SETTING_BLOCK) {
      DVLOG(1) << "AllowAutoplay=false because rule=CONTENT_SETTING_BLOCK";
      if (play_requested) {
        DidBlockContentType(ContentSettingsType::AUTOPLAY);
      }
      return false;
    } else if (setting == CONTENT_SETTING_ALLOW) {
      DVLOG(1) << "AllowAutoplay=true because rule=CONTENT_SETTING_ALLOW";
      return true;
    }
  }

  bool allow = ContentSettingsAgentImpl::AllowAutoplay(play_requested);
  if (allow) {
    DVLOG(1) << "AllowAutoplay=true because "
                "ContentSettingsAgentImpl::AllowAutoplay says so";
  } else {
    if (play_requested) {
      DidBlockContentType(ContentSettingsType::AUTOPLAY);
    }
    DVLOG(1) << "AllowAutoplay=false because "
                "ContentSettingsAgentImpl::AllowAutoplay says so";
  }
  return allow;
}

void BraveContentSettingsAgentImpl::SetShieldsSettings(
    brave_shields::mojom::ShieldsSettingsPtr shields_settings) {
  shields_settings_ = std::move(shields_settings);
}

void BraveContentSettingsAgentImpl::BindBraveShieldsReceiver(
    mojo::PendingAssociatedReceiver<brave_shields::mojom::BraveShields>
        pending_receiver) {
  brave_shields_receivers_.Add(this, std::move(pending_receiver));
}

mojo::AssociatedRemote<brave_shields::mojom::BraveShieldsHost>&
BraveContentSettingsAgentImpl::GetOrCreateBraveShieldsRemote() {
  if (!brave_shields_remote_) {
    render_frame()->GetRemoteAssociatedInterfaces()->GetInterface(
        &brave_shields_remote_);
  }

  DCHECK(brave_shields_remote_.is_bound());
  return brave_shields_remote_;
}

}  // namespace content_settings
