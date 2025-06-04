// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/ui/webui/brave_web_ui_ios_data_source.h"

#include "base/containers/span.h"
#include "base/functional/bind.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/ref_counted_memory.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/ios/browser/ui/webui/brave_url_data_source_ios.h"
#include "ios/chrome/browser/shared/model/url/chrome_url_constants.h"
#include "ios/components/webui/web_ui_url_constants.h"
#include "ios/web/public/browser_state.h"
#include "ios/web/public/web_client.h"
#include "ios/web/public/webui/url_data_source_ios.h"
#include "ios/web/public/webui/web_ui_ios_data_source.h"
#include "ui/base/webui/jstemplate_builder.h"
#include "ui/base/webui/resource_path.h"
#include "ui/base/webui/web_ui_util.h"

// static
BraveWebUIIOSDataSource* BraveWebUIIOSDataSource::Create(
    const std::string& source_name) {
  return new BraveWebUIIOSDataSource(source_name);
}

// static
BraveWebUIIOSDataSource* BraveWebUIIOSDataSource::CreateAndAdd(
    web::BrowserState* browser_state,
    const std::string& source_name) {
  auto* data_source = Create(source_name);
  web::WebUIIOSDataSource::Add(browser_state, data_source);
  return data_source;
}

class BraveWebUIIOSDataSource::BraveInternalDataSource
    : public BraveURLDataSourceIOS {
 public:
  BraveInternalDataSource(BraveWebUIIOSDataSource* parent) : parent_(parent) {}
  ~BraveInternalDataSource() override {}

  std::string GetSource() const override { return parent_->GetSource(); }

  void StartDataRequest(const std::string& path,
                        URLDataSourceIOS::GotDataCallback callback) override {
    return parent_->StartDataRequest(path, std::move(callback));
  }

  std::string GetMimeType(const std::string& path) const override {
    return parent_->GetMimeType(path);
  }

  bool ShouldReplaceExistingSource() const override {
    return parent_->replace_existing_source_;
  }

  bool ShouldReplaceI18nInJS() const override {
    return parent_->ShouldReplaceI18nInJS();
  }

  bool AllowCaching() const override { return false; }

  std::string GetContentSecurityPolicy(
      network::mojom::CSPDirectiveName directive) const override {
    // Check CSP overrides
    if (parent_->csp_overrides_.contains(directive)) {
      return parent_->csp_overrides_.at(directive);
    }

    // Check Frame-Ancestors overrides
    if (directive == network::mojom::CSPDirectiveName::FrameAncestors) {
      std::string frame_ancestors;
      if (parent_->frame_ancestors_.empty()) {
        frame_ancestors += " 'none'";
      }

      for (const GURL& frame_ancestor : parent_->frame_ancestors_) {
        frame_ancestors += " " + frame_ancestor.spec();
      }
      return "frame-ancestors" + frame_ancestors + ";";
    }

    return BraveURLDataSourceIOS::GetContentSecurityPolicy(directive);
  }

  bool ShouldDenyXFrameOptions() const override {
    return parent_->deny_xframe_options_;
  }

  bool ShouldServiceRequest(const GURL& url) const override {
    if (parent_->supported_scheme_.has_value()) {
      return url.SchemeIs(parent_->supported_scheme_.value());
    }

    return BraveURLDataSourceIOS::ShouldServiceRequest(url);
  }

 private:
  raw_ptr<BraveWebUIIOSDataSource> parent_;
};

// WebUIIOSDataSource implementation:

BraveWebUIIOSDataSource::BraveWebUIIOSDataSource(const std::string& source_name)
    : web::WebUIIOSDataSourceImpl(source_name,
                                  new BraveInternalDataSource(this)) {
  CHECK(!source_name.ends_with("://"));
}

BraveWebUIIOSDataSource::~BraveWebUIIOSDataSource() = default;

std::string BraveWebUIIOSDataSource::GetMimeType(
    const std::string& path) const {
  if (base::EndsWith(path, ".css", base::CompareCase::INSENSITIVE_ASCII)) {
    return "text/css";
  }

  if (base::EndsWith(path, ".js", base::CompareCase::INSENSITIVE_ASCII)) {
    return "application/javascript";
  }

  if (base::EndsWith(path, ".ts", base::CompareCase::INSENSITIVE_ASCII)) {
    return "application/typescript";
  }

  if (base::EndsWith(path, ".json", base::CompareCase::INSENSITIVE_ASCII)) {
    return "application/json";
  }

  if (base::EndsWith(path, ".pdf", base::CompareCase::INSENSITIVE_ASCII)) {
    return "application/pdf";
  }

  if (base::EndsWith(path, ".svg", base::CompareCase::INSENSITIVE_ASCII)) {
    return "image/svg+xml";
  }

  if (base::EndsWith(path, ".jpg", base::CompareCase::INSENSITIVE_ASCII)) {
    return "image/jpg";
  }

  if (base::EndsWith(path, ".png", base::CompareCase::INSENSITIVE_ASCII)) {
    return "image/png";
  }

  if (base::EndsWith(path, ".gif", base::CompareCase::INSENSITIVE_ASCII)) {
    return "image/gif";
  }

  if (base::EndsWith(path, ".mp4", base::CompareCase::INSENSITIVE_ASCII)) {
    return "video/mp4";
  }

  if (base::EndsWith(path, ".mp4", base::CompareCase::INSENSITIVE_ASCII)) {
    return "video/mp4";
  }

  if (base::EndsWith(path, ".wasm", base::CompareCase::INSENSITIVE_ASCII)) {
    return "application/wasm";
  }

  if (base::EndsWith(path, ".woff2", base::CompareCase::INSENSITIVE_ASCII)) {
    return "application/font-woff2";
  }

  return "text/html";
}

// Brave CSP's & Security implementation:

void BraveWebUIIOSDataSource::SetSupportedScheme(std::string_view scheme) {
  CHECK(!supported_scheme_.has_value());
  supported_scheme_ = scheme;
}

void BraveWebUIIOSDataSource::OverrideContentSecurityPolicy(
    network::mojom::CSPDirectiveName directive,
    const std::string& value) {
  csp_overrides_.insert_or_assign(directive, value);
}

void BraveWebUIIOSDataSource::AddFrameAncestor(const GURL& frame_ancestor) {
  // Do not allow a wildcard to be a frame ancestor or it will allow any website
  // to embed the WebUI.
  CHECK(frame_ancestor.SchemeIs(kChromeUIScheme) ||
        frame_ancestor.SchemeIs(kChromeUIUntrustedScheme));
  frame_ancestors_.insert(frame_ancestor);
}

void BraveWebUIIOSDataSource::DisableTrustedTypesCSP() {
  // TODO(crbug.com/40137141): Trusted Type remaining WebUI
  // This removes require-trusted-types-for and trusted-types directives
  // from the CSP header.
  OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::RequireTrustedTypesFor, std::string());
  OverrideContentSecurityPolicy(network::mojom::CSPDirectiveName::TrustedTypes,
                                std::string());
}
