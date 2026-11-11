/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <optional>

#include "services/network/public/cpp/ip_address_space_util.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/url_loader_factory.mojom.h"
#include "url/gurl.h"

namespace {

bool ShouldRejectLocalImageRequest(const GURL& image_url) {
  std::optional<network::mojom::IPAddressSpace> address_space =
      network::GetAddressSpaceFromUrl(image_url);
  return address_space &&
         (*address_space == network::mojom::IPAddressSpace::kLoopback ||
          *address_space == network::mojom::IPAddressSpace::kLocal);
}

void SetSanitizedImageLoaderOptions(network::SimpleURLLoader* loader,
                                    const GURL& image_url) {
  loader->SetURLLoaderFactoryOptions(
      network::mojom::kURLLoadOptionBlockLocalRequest |
      (image_url.host() == "drive.google.com"
           ? 0u
           : network::mojom::kURLLoadOptionBlockAllCookies));
}

}  // namespace

#include <chrome/browser/ui/webui/sanitized_image/sanitized_image_source.cc>
