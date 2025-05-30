// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/ui/webui/brave_url_data_source_ios.h"

#include "base/no_destructor.h"
#include "base/strings/strcat.h"
#include "ios/components/webui/web_ui_url_constants.h"

BraveURLDataSourceIOS::BraveURLDataSourceIOS() {}
BraveURLDataSourceIOS::~BraveURLDataSourceIOS() = default;

std::string BraveURLDataSourceIOS::GetContentSecurityPolicyObjectSrc() const {
  std::string csp_header;

  // Same as on Desktop
  // content/browser/webui/url_data_manager_backend.cc
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

std::string BraveURLDataSourceIOS::GetContentSecurityPolicyFrameSrc() const {
  std::string frame_src =
      GetContentSecurityPolicy(network::mojom::CSPDirectiveName::FrameSrc);
  if (!frame_src.empty()) {
    return frame_src;
  }

  // The default for iOS is `frame-src 'none';` in url_data_manager_ios_backend.mm
  // Desktop never sets a frame-src
  return "";
}

std::string BraveURLDataSourceIOS::GetContentSecurityPolicy(
    network::mojom::CSPDirectiveName directive) const {
  bool is_untrusted = GURL(GetSource()).SchemeIs(kChromeUIUntrustedScheme);

  // Default policies matching Chromium Desktop
  // content/public/browser/url_data_source.cc
  switch (directive) {
    case network::mojom::CSPDirectiveName::ChildSrc:
      return "child-src 'none';";
    case network::mojom::CSPDirectiveName::DefaultSrc:
      return is_untrusted ? "default-src 'self';" : std::string();
    case network::mojom::CSPDirectiveName::ObjectSrc:
      return "object-src 'none';";
    case network::mojom::CSPDirectiveName::ScriptSrc:
      // Note: Do not add 'unsafe-eval' here. Instead override CSP for the
      // specific pages that need it, see context http://crbug.com/525224.
      return is_untrusted
                 ? base::StrCat({"script-src", kChromeUIUntrustedScheme,
                                 url::kStandardSchemeSeparator,
                                 "resources 'self';"})
                 : "script-src chrome://resources 'self';";
    case network::mojom::CSPDirectiveName::FrameAncestors:
      return "frame-ancestors 'none';";
    case network::mojom::CSPDirectiveName::RequireTrustedTypesFor:
      return "require-trusted-types-for 'script';";
    case network::mojom::CSPDirectiveName::TrustedTypes:
      return "trusted-types;";
    case network::mojom::CSPDirectiveName::BaseURI:
      return is_untrusted ? "base-uri 'none';" : std::string();
    case network::mojom::CSPDirectiveName::FormAction:
      return is_untrusted ? "form-action 'none';" : std::string();
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
    case network::mojom::CSPDirectiveName::ScriptSrcV2:
    case network::mojom::CSPDirectiveName::ScriptSrcAttr:
    case network::mojom::CSPDirectiveName::ScriptSrcElem:
    case network::mojom::CSPDirectiveName::StyleSrc:
    case network::mojom::CSPDirectiveName::StyleSrcAttr:
    case network::mojom::CSPDirectiveName::StyleSrcElem:
    case network::mojom::CSPDirectiveName::UpgradeInsecureRequests:
    case network::mojom::CSPDirectiveName::TreatAsPublicAddress:
    case network::mojom::CSPDirectiveName::WorkerSrc:
    case network::mojom::CSPDirectiveName::ReportTo:
    case network::mojom::CSPDirectiveName::Unknown:
      return std::string();
  }
}
