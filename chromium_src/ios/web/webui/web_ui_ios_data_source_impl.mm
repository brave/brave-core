/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "ios/web/webui/web_ui_ios_data_source_impl.h"

#include "ios/web/public/webui/web_ui_ios_data_source.h"
#include "services/network/public/mojom/content_security_policy.mojom.h"

namespace {
const char kChromeUIScheme[] = "chrome";
const char kChromeUIUntrustedScheme[] = "chrome-untrusted";
}  // namespace

namespace web {

void WebUIIOSDataSourceImpl::OverrideContentSecurityPolicy(
    network::mojom::CSPDirectiveName directive,
    const std::string& value) {
  csp_overrides_.insert_or_assign(directive, value);
}

void WebUIIOSDataSourceImpl::AddFrameAncestor(const GURL& frame_ancestor) {
  // Do not allow a wildcard to be a frame ancestor or it will allow any website
  // to embed the WebUI.
  CHECK(frame_ancestor.SchemeIs(kChromeUIScheme) ||
        frame_ancestor.SchemeIs(kChromeUIUntrustedScheme));
  frame_ancestors_.insert(frame_ancestor);
}

void WebUIIOSDataSourceImpl::DisableTrustedTypesCSP() {
  // TODO(crbug.com/40137141): Trusted Type remaining WebUI
  // This removes require-trusted-types-for and trusted-types directives
  // from the CSP header.
  OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::RequireTrustedTypesFor, std::string());
  OverrideContentSecurityPolicy(network::mojom::CSPDirectiveName::TrustedTypes,
                                std::string());
}

}  // namespace web

#define ShouldDenyXFrameOptions                                   \
  Dummy() const;                                                  \
  std::string GetContentSecurityPolicy(                           \
      network::mojom::CSPDirectiveName directive) const override; \
  bool ShouldDenyXFrameOptions

#include "src/ios/web/webui/web_ui_ios_data_source_impl.mm"

#undef ShouldDenyXFrameOptions

namespace web {

bool WebUIIOSDataSourceImpl::InternalDataSource::Dummy() const {
  return false;
}

std::string
WebUIIOSDataSourceImpl::InternalDataSource::GetContentSecurityPolicy(
    network::mojom::CSPDirectiveName directive) const {
  if (parent_->csp_overrides_.contains(directive)) {
    return parent_->csp_overrides_.at(directive);
  } else if (directive == network::mojom::CSPDirectiveName::FrameAncestors) {
    std::string frame_ancestors;
    if (parent_->frame_ancestors_.size() == 0) {
      frame_ancestors += " 'none'";
    }
    for (const GURL& frame_ancestor : parent_->frame_ancestors_) {
      frame_ancestors += " " + frame_ancestor.spec();
    }
    return "frame-ancestors" + frame_ancestors + ";";
  }

  return URLDataSourceIOS::GetContentSecurityPolicy(directive);
}

}  // namespace web
