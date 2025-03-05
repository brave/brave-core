// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/padded_image_source.h"

#include <memory>
#include <string>
#include <utility>

#include "brave/brave_domains/service_domains.h"
#include "brave/components/brave_private_cdn/private_cdn_helper.h"
#include "chrome/browser/profiles/profile.h"
#include "net/base/net_errors.h"
#include "services/network/public/cpp/simple_url_loader.h"

PaddedImageSource::PaddedImageSource(Profile* profile)
    : SanitizedImageSource(profile),
      pcdn_domain_(brave_domains::GetServicesDomain("pcdn")) {}

PaddedImageSource::PaddedImageSource(
    Profile* profile,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    std::unique_ptr<DataDecoderDelegate> delegate,
    std::string pcdn_domain)
    : SanitizedImageSource(profile, url_loader_factory, std::move(delegate)),
      pcdn_domain_(pcdn_domain) {}

PaddedImageSource::~PaddedImageSource() = default;

void PaddedImageSource::OnImageLoaded(
    std::unique_ptr<network::SimpleURLLoader> loader,
    RequestAttributes request_attributes,
    content::URLDataSource::GotDataCallback callback,
    std::unique_ptr<std::string> body) {
  if (loader->NetError() != net::OK || !body) {
    std::move(callback).Run(nullptr);
    return;
  }

  if (request_attributes.image_url.host() == pcdn_domain_ &&
      request_attributes.image_url.path().ends_with(".pad")) {
    std::string_view body_payload(body->data(), body->size());
    if (!brave::PrivateCdnHelper::GetInstance()->RemovePadding(&body_payload)) {
      std::move(callback).Run(nullptr);
      return;
    }

    *body = body_payload;
  }

  SanitizedImageSource::OnImageLoaded(std::move(loader), request_attributes,
                                      std::move(callback), std::move(body));
}
