// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/webui/sanitized_image_source.h"

#include <memory>
#include <string>
#include <utility>

#include "base/strings/strcat.h"
#include "brave/brave_domains/service_domains.h"
#include "brave/components/brave_private_cdn/private_cdn_helper.h"

#define SanitizedImageSource SanitizedImageSource_Chromium

#include <chrome/browser/ui/webui/sanitized_image_source.cc>

#undef SanitizedImageSource

SanitizedImageSource::~SanitizedImageSource() = default;

void SanitizedImageSource::OnImageLoaded(
    std::unique_ptr<network::SimpleURLLoader> loader,
    RequestAttributes request_attributes,
    content::URLDataSource::GotDataCallback callback,
    std::unique_ptr<std::string> body) {  // Lazily initialize the pcdn domain
  if (pcdn_domain_.empty()) {
    pcdn_domain_ = brave_domains::GetServicesDomain("pcdn");
  }

  if (loader->NetError() == net::OK && body &&
      request_attributes.image_url.host_piece() == pcdn_domain_ &&
      request_attributes.image_url.path_piece().ends_with(".pad")) {
    std::string_view body_payload(body->data(), body->size());
    if (!brave::private_cdn::RemovePadding(&body_payload)) {
      std::move(callback).Run(nullptr);
      return;
    }

    *body = body_payload;
  }

  SanitizedImageSource_Chromium::OnImageLoaded(
      std::move(loader), request_attributes, std::move(callback),
      std::move(body));
}
