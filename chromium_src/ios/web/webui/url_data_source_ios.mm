// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ios/web/public/webui/url_data_source_ios.h"

#include "base/containers/span.h"
#include "base/no_destructor.h"
#include "base/strings/strcat.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "ios/chrome/browser/shared/model/url/chrome_url_constants.h"
#include "services/network/public/mojom/content_security_policy.mojom.h"

namespace {

const char kChromeUIUntrustedScheme[] = "chrome-untrusted";

// A chrome-untrusted data source's name starts with chrome-untrusted://.
bool IsChromeUntrustedDataSource(const web::URLDataSourceIOS* source) {
  static const base::NoDestructor<std::string> kChromeUntrustedSourceNamePrefix(
      base::StrCat({kChromeUIUntrustedScheme, url::kStandardSchemeSeparator}));

  return base::StartsWith(source->GetSource(),
                          *kChromeUntrustedSourceNamePrefix,
                          base::CompareCase::SENSITIVE);
}

}  // namespace

namespace web {
bool URLDataSourceIOS::ShouldAddContentSecurityPolicy() const {
  return true;
}

bool URLDataSourceIOS::ShouldServiceRequest(const GURL& url) const {
  return URLDataSourceIOS::ShouldServiceRequest_ChromiumImpl(url);
}

std::string URLDataSourceIOS::GetContentSecurityPolicyObjectSrc() const {
  if (ShouldAddContentSecurityPolicy()) {
    std::string csp_header;

    const network::mojom::CSPDirectiveName kAllDirectives[] = {
        network::mojom::CSPDirectiveName::BaseURI,
        network::mojom::CSPDirectiveName::ChildSrc,
        network::mojom::CSPDirectiveName::ConnectSrc,
        network::mojom::CSPDirectiveName::DefaultSrc,
        network::mojom::CSPDirectiveName::FencedFrameSrc,
        network::mojom::CSPDirectiveName::FormAction,
        network::mojom::CSPDirectiveName::FontSrc,
        network::mojom::CSPDirectiveName::ImgSrc,
        network::mojom::CSPDirectiveName::MediaSrc,
        network::mojom::CSPDirectiveName::ObjectSrc,
        network::mojom::CSPDirectiveName::RequireTrustedTypesFor,
        network::mojom::CSPDirectiveName::ScriptSrc,
        network::mojom::CSPDirectiveName::StyleSrc,
        network::mojom::CSPDirectiveName::TrustedTypes,
        network::mojom::CSPDirectiveName::WorkerSrc};

    for (auto& directive : kAllDirectives) {
      csp_header.append(GetContentSecurityPolicy(directive));
    }

    // TODO(crbug.com/40118579): Both CSP frame ancestors and XFO headers may be
    // added to the response but frame ancestors would take precedence. In the
    // future, XFO will be removed so when that happens remove the check and
    // always add frame ancestors.
    if (ShouldDenyXFrameOptions()) {
      csp_header.append(GetContentSecurityPolicy(
          network::mojom::CSPDirectiveName::FrameAncestors));
    }

    return csp_header;
  }

  return URLDataSourceIOS::GetContentSecurityPolicyObjectSrc_ChromiumImpl();
}

std::string URLDataSourceIOS::GetContentSecurityPolicyFrameSrc() const {
  std::string frame_src =
      GetContentSecurityPolicy(network::mojom::CSPDirectiveName::FrameSrc);
  if (!frame_src.empty()) {
    return frame_src;
  }

  return "frame-src 'none';";
}

std::string URLDataSourceIOS::GetContentSecurityPolicy(
    network::mojom::CSPDirectiveName directive) const {
  switch (directive) {
    case network::mojom::CSPDirectiveName::ChildSrc:
      return "child-src 'none';";
    case network::mojom::CSPDirectiveName::DefaultSrc:
      return IsChromeUntrustedDataSource(this) ? "default-src 'self';"
                                               : std::string();
    case network::mojom::CSPDirectiveName::ObjectSrc:
      return "object-src 'none';";
    case network::mojom::CSPDirectiveName::ScriptSrc:
      // Note: Do not add 'unsafe-eval' here. Instead override CSP for the
      // specific pages that need it, see context http://crbug.com/525224.
      return IsChromeUntrustedDataSource(this)
                 ? "script-src chrome-untrusted://resources 'self';"
                 : "script-src chrome://resources 'self';";
    case network::mojom::CSPDirectiveName::FrameAncestors:
      return "frame-ancestors 'none';";
    case network::mojom::CSPDirectiveName::RequireTrustedTypesFor:
      return "require-trusted-types-for 'script';";
    case network::mojom::CSPDirectiveName::TrustedTypes:
      return "trusted-types;";
    case network::mojom::CSPDirectiveName::BaseURI:
      return IsChromeUntrustedDataSource(this) ? "base-uri 'none';"
                                               : std::string();
    case network::mojom::CSPDirectiveName::FormAction:
      return IsChromeUntrustedDataSource(this) ? "form-action 'none';"
                                               : std::string();
    case network::mojom::CSPDirectiveName::BlockAllMixedContent:
    case network::mojom::CSPDirectiveName::ConnectSrc:
    case network::mojom::CSPDirectiveName::FencedFrameSrc:
    case network::mojom::CSPDirectiveName::FrameSrc:
    case network::mojom::CSPDirectiveName::FontSrc:
    case network::mojom::CSPDirectiveName::ImgSrc:
    case network::mojom::CSPDirectiveName::ManifestSrc:
    case network::mojom::CSPDirectiveName::MediaSrc:
    case network::mojom::CSPDirectiveName::ReportURI:
    case network::mojom::CSPDirectiveName::Sandbox:
    case network::mojom::CSPDirectiveName::ScriptSrcAttr:
    case network::mojom::CSPDirectiveName::ScriptSrcElem:
    case network::mojom::CSPDirectiveName::StyleSrc:
    case network::mojom::CSPDirectiveName::StyleSrcAttr:
    case network::mojom::CSPDirectiveName::StyleSrcElem:
    case network::mojom::CSPDirectiveName::UpgradeInsecureRequests:
    case network::mojom::CSPDirectiveName::TreatAsPublicAddress:
    case network::mojom::CSPDirectiveName::WorkerSrc:
    case network::mojom::CSPDirectiveName::ReportTo:
    case network::mojom::CSPDirectiveName::NavigateTo:
    case network::mojom::CSPDirectiveName::Unknown:
      return std::string();
  }
}

}  // namespace web

#define GetContentSecurityPolicyObjectSrc \
  GetContentSecurityPolicyObjectSrc_ChromiumImpl

#define ShouldServiceRequest ShouldServiceRequest_ChromiumImpl

#include "src/ios/web/webui/url_data_source_ios.mm"

#undef ShouldServiceRequest
#undef GetContentSecurityPolicyObjectSrc
