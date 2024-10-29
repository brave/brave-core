// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ios/web/webui/web_ui_ios_data_source_impl.h"

#include "base/memory/raw_ptr.h"
#include "brave/ios/browser/ui/webui/brave_url_data_source_ios.h"
#include "ios/chrome/browser/shared/model/url/chrome_url_constants.h"
#include "ios/components/webui/web_ui_url_constants.h"

// static
web::WebUIIOSDataSource* BraveWebUIIOSDataSource::Create(
    const std::string& source_name) {
  return new BraveWebUIIOSDataSource(source_name);
}

class BraveWebUIIOSDataSource::InternalDataSource
    : public BraveURLDataSourceIOS {
 public:
  InternalDataSource(web::WebUIIOSDataSourceImpl* parent) : parent_(parent) {}
  ~InternalDataSource() override {}

  std::string GetSource() const override { return parent_->GetSource(); }

  void StartDataRequest(
      const std::string& path,
      web::URLDataSourceIOS::GotDataCallback callback) override {
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

  bool ShouldDenyXFrameOptions() const override {
    return parent_->deny_xframe_options_;
  }

  std::string GetContentSecurityPolicy(
      network::mojom::CSPDirectiveName directive) const override {
    auto* parent = static_cast<BraveWebUIIOSDataSource*>(parent_);

    // Check CSP overrides
    if (parent->csp_overrides_.contains(directive)) {
      return parent->csp_overrides_.at(directive);
    }

    // Check Frame-Ancestors overrides
    if (directive == network::mojom::CSPDirectiveName::FrameAncestors) {
      std::string frame_ancestors;
      if (parent->frame_ancestors_.empty()) {
        frame_ancestors += " 'none'";
      }

      for (const GURL& frame_ancestor : parent->frame_ancestors_) {
        frame_ancestors += " " + frame_ancestor.spec();
      }
      return "frame-ancestors" + frame_ancestors + ";";
    }

    return BraveURLDataSourceIOS::GetContentSecurityPolicy(directive);
  }

 private:
  raw_ptr<web::WebUIIOSDataSourceImpl> parent_;
};

// WebUIIOSDataSource implementation:

BraveWebUIIOSDataSource::BraveWebUIIOSDataSource(const std::string& source_name)
    : web::WebUIIOSDataSourceImpl(source_name) {
  CHECK(!source_name.ends_with("://"));
}

BraveWebUIIOSDataSource::~BraveWebUIIOSDataSource() = default;

// Brave CSP's & Security implementation:

void BraveWebUIIOSDataSource::OverrideContentSecurityPolicy(
    network::mojom::CSPDirectiveName directive,
    const std::string& value) {
  csp_overrides_.insert_or_assign(directive, value);
}

void BraveWebUIIOSDataSource::AddFrameAncestor(const GURL& frame_ancestor) {
  // Do not allow a wildcard to be a frame ancestor or it will allow any website
  // to embed the WebUI.
  CHECK(frame_ancestor.SchemeIs(kChromeUIScheme));
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

#define URLDataSourceIOSImpl(SOURCE_NAME, DATA_SOURCE) \
  URLDataSourceIOSImpl(SOURCE_NAME,                    \
                       new BraveWebUIIOSDataSource::InternalDataSource(this))
#include "src/ios/web/webui/web_ui_ios_data_source_impl.mm"
#undef URLDataSourceIOSImpl
