// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "content/browser/webauth/webauth_request_security_checker.h"

#include "content/public/browser/browser_context.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/common/content_client.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace {

scoped_refptr<network::SharedURLLoaderFactory> GetUrlLoaderFactory(
    content::RenderFrameHost* render_frame_host) {
  CHECK(render_frame_host);
  content::BrowserContext* browser_context =
      render_frame_host->GetBrowserContext();
  if (!browser_context || !browser_context->IsTor()) {
    return content::GetContentClient()
        ->browser()
        ->GetSystemSharedURLLoaderFactory();
  }
  if (!browser_context->GetDefaultStoragePartition()) {
    return nullptr;
  }
  return browser_context->GetDefaultStoragePartition()
      ->GetURLLoaderFactoryForBrowserProcess();
}

}  // namespace

#define GetSystemSharedURLLoaderFactory()                                    \
  GetSystemSharedURLLoaderFactory() ? GetUrlLoaderFactory(render_frame_host) \
                                    : GetUrlLoaderFactory(render_frame_host)

#include "src/content/browser/webauth/webauth_request_security_checker.cc"

#undef GetSystemSharedURLLoaderFactory
