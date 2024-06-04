// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/ui/webui/brave_webui_untrusted_source.h"

#include "base/containers/span.h"
#include "base/files/file_util.h"
#include "base/memory/ref_counted_memory.h"
#include "base/no_destructor.h"
#include "base/strings/strcat.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "ios/chrome/browser/shared/model/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/shared/model/url/chrome_url_constants.h"
#include "services/network/public/mojom/content_security_policy.mojom.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/template_expressions.h"
#include "url/url_util.h"

namespace {

const char kChromeUIScheme[] = "chrome";
const char kChromeUIUntrustedScheme[] = "chrome-untrusted";
const char kChromeUIUntrustedNewTabPageUrl[] = "chrome-untrusted://newtab/";

// A chrome-untrusted data source's name starts with chrome-untrusted://.
bool IsChromeUntrustedDataSource(const web::URLDataSourceIOS* source) {
  static const base::NoDestructor<std::string> kChromeUntrustedSourceNamePrefix(
      base::StrCat({kChromeUIUntrustedScheme, url::kStandardSchemeSeparator}));

  return base::StartsWith(source->GetSource(),
                          *kChromeUntrustedSourceNamePrefix,
                          base::CompareCase::SENSITIVE);
}

std::string GetContentSecurityPolicy(
    const web::URLDataSourceIOS* source,
    network::mojom::CSPDirectiveName directive) {
  switch (directive) {
    case network::mojom::CSPDirectiveName::ChildSrc:
      return "child-src 'none';";
    case network::mojom::CSPDirectiveName::DefaultSrc:
      return IsChromeUntrustedDataSource(source) ? "default-src 'self';"
                                                 : std::string();
    case network::mojom::CSPDirectiveName::ObjectSrc:
      return "object-src 'none';";
    case network::mojom::CSPDirectiveName::ScriptSrc:
      // Note: Do not add 'unsafe-eval' here. Instead override CSP for the
      // specific pages that need it, see context http://crbug.com/525224.
      return IsChromeUntrustedDataSource(source)
                 ? "script-src chrome-untrusted://resources 'self';"
                 : "script-src chrome://resources 'self';";
    case network::mojom::CSPDirectiveName::FrameAncestors:
      return "frame-ancestors 'none';";
    case network::mojom::CSPDirectiveName::RequireTrustedTypesFor:
      return "require-trusted-types-for 'script';";
    case network::mojom::CSPDirectiveName::TrustedTypes:
      return "trusted-types;";
    case network::mojom::CSPDirectiveName::BaseURI:
      return IsChromeUntrustedDataSource(source) ? "base-uri 'none';"
                                                 : std::string();
    case network::mojom::CSPDirectiveName::FormAction:
      return IsChromeUntrustedDataSource(source) ? "form-action 'none';"
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

}  // namespace

UntrustedSource::UntrustedSource(ChromeBrowserState* browser_state) {}

UntrustedSource::~UntrustedSource() {}

std::string UntrustedSource::GetContentSecurityPolicy(
    network::mojom::CSPDirectiveName directive) const {
  if (csp_overrides_.contains(directive)) {
    return csp_overrides_.at(directive);
  } else if (directive == network::mojom::CSPDirectiveName::FrameAncestors) {
    std::string frame_ancestors;
    if (frame_ancestors_.size() == 0) {
      frame_ancestors += " 'none'";
    }
    for (const GURL& frame_ancestor : frame_ancestors_) {
      frame_ancestors += " " + frame_ancestor.spec();
    }
    return "frame-ancestors" + frame_ancestors + ";";
  }

  switch (directive) {
    case network::mojom::CSPDirectiveName::ScriptSrc:
      return "script-src 'self' 'unsafe-inline' https:;";
    case network::mojom::CSPDirectiveName::ChildSrc:
      return "child-src https:;";
    case network::mojom::CSPDirectiveName::DefaultSrc:
      // TODO(crbug.com/40693567): Audit and tighten CSP.
      return std::string();
    case network::mojom::CSPDirectiveName::FrameAncestors:
      return base::StringPrintf("frame-ancestors %s", kChromeUINewTabURL);
    case network::mojom::CSPDirectiveName::RequireTrustedTypesFor:
      return std::string();
    case network::mojom::CSPDirectiveName::TrustedTypes:
      return std::string();
    case network::mojom::CSPDirectiveName::FormAction:
      return "form-action https://ogs.google.com https://*.corp.google.com;";
    default:
      return ::GetContentSecurityPolicy(this, directive);
  }
}

void UntrustedSource::OverrideContentSecurityPolicy(
    network::mojom::CSPDirectiveName directive,
    const std::string& value) {
  csp_overrides_.insert_or_assign(directive, value);
}

void UntrustedSource::AddFrameAncestor(const GURL& frame_ancestor) {
  // Do not allow a wildcard to be a frame ancestor or it will allow any website
  // to embed the WebUI.
  CHECK(frame_ancestor.SchemeIs(kChromeUIScheme) ||
        frame_ancestor.SchemeIs(kChromeUIUntrustedScheme));
  frame_ancestors_.insert(frame_ancestor);
}

void UntrustedSource::DisableTrustedTypesCSP() {
  // TODO(crbug.com/40137141): Trusted Type remaining WebUI
  // This removes require-trusted-types-for and trusted-types directives
  // from the CSP header.
  OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::RequireTrustedTypesFor, std::string());
  OverrideContentSecurityPolicy(network::mojom::CSPDirectiveName::TrustedTypes,
                                std::string());
}

std::string UntrustedSource::GetSource() const {
  return kChromeUIUntrustedNewTabPageUrl;
}

std::string UntrustedSource::GetContentSecurityPolicyObjectSrc() const {
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
        //      network::mojom::CSPDirectiveName::FrameSrc,
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

  // See url_data_manager_ios_backend.mm chromium_src override for more details
  return web::URLDataSourceIOS::GetContentSecurityPolicyObjectSrc();
}

std::string UntrustedSource::GetContentSecurityPolicyFrameSrc() const {
  if (csp_overrides_.contains(network::mojom::CSPDirectiveName::FrameSrc)) {
    return csp_overrides_.at(network::mojom::CSPDirectiveName::FrameSrc);
  }

  std::string frame_src =
      GetContentSecurityPolicy(network::mojom::CSPDirectiveName::FrameSrc);
  if (!frame_src.empty()) {
    return frame_src;
  }

  // See url_data_manager_ios_backend.mm chromium_src override for more details
  return web::URLDataSourceIOS::GetContentSecurityPolicyFrameSrc();
}

void UntrustedSource::StartDataRequest(const std::string& path,
                                       GotDataCallback callback) {
  std::move(callback).Run(base::MakeRefCounted<base::RefCountedString>());
}

std::string UntrustedSource::GetMimeType(const std::string& path) const {
  if (base::EndsWith(path, ".js", base::CompareCase::INSENSITIVE_ASCII)) {
    return "application/javascript";
  }
  if (base::EndsWith(path, ".jpg", base::CompareCase::INSENSITIVE_ASCII)) {
    return "image/jpg";
  }

  return "text/html";
}

bool UntrustedSource::AllowCaching() const {
  return false;
}

bool UntrustedSource::ShouldDenyXFrameOptions() const {
  return true;
}

bool UntrustedSource::ShouldServiceRequest(const GURL& url) const {
  if (!url.SchemeIs(kChromeUIUntrustedScheme) || !url.has_path()) {
    return false;
  }
  return true;
}
