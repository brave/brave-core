/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/debug/dump_without_crashing.h"

#include <content/public/browser/content_browser_client.cc>

namespace content {

std::string ContentBrowserClient::GetEffectiveUserAgent(
    BrowserContext* browser_context,
    const GURL& url) {
  return std::string();
}

bool ContentBrowserClient::AllowWorkerFingerprinting(
    const GURL& url,
    BrowserContext* browser_context) {
  return true;
}

std::optional<base::UnguessableToken>
ContentBrowserClient::GetEphemeralStorageToken(
    RenderFrameHost* render_frame_host,
    const url::Origin& origin) {
  return std::nullopt;
}

brave_shields::mojom::ShieldsSettingsPtr
ContentBrowserClient::WorkerGetBraveShieldSettings(
    const GURL& url,
    BrowserContext* browser_context,
    const StoragePartitionConfig* storage_partition_config) {
  // BraveContentBrowserClient should implement this. It's possible this is
  // reached somehow, add dumps to see if it's true.
  base::debug::DumpWithoutCrashing();
  return brave_shields::mojom::ShieldsSettingsPtr();
}

void ContentBrowserClient::CreateWebSocketWithFrameId(
    RenderFrameHost* frame,
    WebSocketFactory factory,
    const GURL& url,
    const net::SiteForCookies& site_for_cookies,
    const std::optional<std::string>& user_agent,
    mojo::PendingRemote<network::mojom::WebSocketHandshakeClient>
        handshake_client,
    WebSocketOptions options,
    int /*process_id*/,
    const url::Origin& /*initiator_origin*/) {
  // The frameless SharedWorker/ServiceWorker context (crbug.com/40195467) is
  // only consumed by the Brave override; the default path ignores it and
  // forwards to the regular CreateWebSocket.
  CreateWebSocket(frame, std::move(factory), url, site_for_cookies, user_agent,
                  std::move(handshake_client), std::move(options));
}

std::optional<GURL> ContentBrowserClient::SanitizeURL(content::RenderFrameHost*,
                                                      const GURL& url) {
  return std::nullopt;
}

bool ContentBrowserClient::IsWindowsRecallDisabled() {
  return false;
}

bool ContentBrowserClient::ShouldInheritStoragePartition(
    const content::StoragePartitionConfig& partition_config) const {
  return false;
}

bool ContentBrowserClient::ShouldUseDefaultHostZoomMapForStoragePartition(
    const content::StoragePartitionConfig& partition_config) const {
  return false;
}

mojo::PendingRemote<local_ai::mojom::AsrSession>
ContentBrowserClient::GetAsrSession() {
  return {};
}

}  // namespace content
