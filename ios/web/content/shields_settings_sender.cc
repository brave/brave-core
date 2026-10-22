// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/web/content/shields_settings_sender.h"

#include <string>

#include "base/containers/flat_map.h"
#include "base/no_destructor.h"
#include "base/token.h"
#include "brave/components/brave_shields/core/common/brave_shields.mojom.h"
#include "brave/components/brave_shields/core/common/shields_settings.mojom.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "mojo/public/cpp/bindings/associated_remote.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"
#include "url/gurl.h"

namespace brave_shields {

namespace {

// Random per site and per launch, so fingerprints differ across sites and
// sessions but stay stable within a site. Desktop persists these instead.
base::Token GetFarblingToken(const GURL& url) {
  static base::NoDestructor<base::flat_map<std::string, base::Token>> tokens;
  std::string site = net::registry_controlled_domains::GetDomainAndRegistry(
      url, net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);
  if (site.empty()) {
    site = url.host();
  }
  auto [it, inserted] = tokens->try_emplace(site);
  if (inserted) {
    it->second = base::Token::CreateRandom();
  }
  return it->second;
}

}  // namespace

ShieldsSettingsSender::ShieldsSettingsSender(content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents),
      content::WebContentsUserData<ShieldsSettingsSender>(*web_contents) {}

ShieldsSettingsSender::~ShieldsSettingsSender() = default;

void ShieldsSettingsSender::ReadyToCommitNavigation(
    content::NavigationHandle* navigation_handle) {
  content::RenderFrameHost* rfh = navigation_handle->GetRenderFrameHost();
  content::RenderFrameHost* parent =
      navigation_handle->GetParentFrameOrOuterDocument();
  const GURL primary_url =
      parent ? parent->GetOutermostMainFrame()->GetLastCommittedURL()
             : navigation_handle->GetURL();

  auto settings = mojom::ShieldsSettings::New();
  // TODO(blink-ios): Respect the per-site fingerprinting setting from Swift.
  if (primary_url.SchemeIsHTTPOrHTTPS()) {
    settings->farbling_level = mojom::FarblingLevel::BALANCED;
    settings->farbling_token = GetFarblingToken(primary_url);
  }
  settings->reduce_language = true;

  mojo::AssociatedRemote<mojom::BraveShields> agent;
  rfh->GetRemoteAssociatedInterfaces()->GetInterface(&agent);
  agent->SetShieldsSettings(std::move(settings));
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(ShieldsSettingsSender);

}  // namespace brave_shields
