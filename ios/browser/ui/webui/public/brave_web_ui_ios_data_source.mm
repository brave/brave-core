// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/ui/webui/public/brave_web_ui_ios_data_source.h"

#include "base/containers/span.h"
#include "base/functional/bind.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/ref_counted_memory.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/ios/browser/ui/webui/public/brave_url_data_source_ios.h"
#include "ios/chrome/browser/shared/model/url/chrome_url_constants.h"
#include "ios/components/webui/web_ui_url_constants.h"
#include "ios/web/public/web_client.h"
#include "ios/web/public/webui/web_ui_ios_data_source.h"
#include "ui/base/webui/jstemplate_builder.h"
#include "ui/base/webui/resource_path.h"
#include "ui/base/webui/web_ui_util.h"

// static
web::WebUIIOSDataSource* BraveWebUIIOSDataSource::Create(
    const std::string& source_name) {
  return new BraveWebUIIOSDataSource(source_name);
}

class BraveWebUIIOSDataSource::InternalDataSource
    : public BraveURLDataSourceIOS {
 public:
  InternalDataSource(BraveWebUIIOSDataSource* parent) : parent_(parent) {}
  ~InternalDataSource() override {}

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
    : URLDataSourceIOSImpl(source_name, new InternalDataSource(this)),
      default_resource_(-1),
      use_strings_js_(false),
      deny_xframe_options_(true),
      load_time_data_defaults_added_(false),
      replace_existing_source_(true),
      should_replace_i18n_in_js_(false) {
  CHECK(!source_name.ends_with("://"));
}

BraveWebUIIOSDataSource::~BraveWebUIIOSDataSource() = default;

void BraveWebUIIOSDataSource::AddString(const std::string& name,
                                        const std::u16string& value) {
  localized_strings_.Set(name, value);
  replacements_[name] = base::UTF16ToUTF8(value);
}

void BraveWebUIIOSDataSource::AddString(const std::string& name,
                                        const std::string& value) {
  localized_strings_.Set(name, value);
  replacements_[name] = value;
}

void BraveWebUIIOSDataSource::AddLocalizedString(const std::string& name,
                                                 int ids) {
  localized_strings_.Set(name, web::GetWebClient()->GetLocalizedString(ids));
  replacements_[name] =
      base::UTF16ToUTF8(web::GetWebClient()->GetLocalizedString(ids));
}

void BraveWebUIIOSDataSource::AddLocalizedStrings(
    const base::Value::Dict& localized_strings) {
  localized_strings_.Merge(localized_strings.Clone());
  ui::TemplateReplacementsFromDictionaryValue(localized_strings,
                                              &replacements_);
}

void BraveWebUIIOSDataSource::AddLocalizedStrings(
    base::span<const webui::LocalizedString> strings) {
  for (const auto& str : strings) {
    AddLocalizedString(str.name, str.id);
  }
}

void BraveWebUIIOSDataSource::AddBoolean(const std::string& name, bool value) {
  localized_strings_.Set(name, value);
}

void BraveWebUIIOSDataSource::UseStringsJs() {
  use_strings_js_ = true;
}

void BraveWebUIIOSDataSource::EnableReplaceI18nInJS() {
  should_replace_i18n_in_js_ = true;
}

bool BraveWebUIIOSDataSource::ShouldReplaceI18nInJS() const {
  return should_replace_i18n_in_js_;
}

void BraveWebUIIOSDataSource::AddResourcePath(const std::string& path,
                                              int resource_id) {
  path_to_idr_map_[path] = resource_id;
}

void BraveWebUIIOSDataSource::AddResourcePaths(
    base::span<const webui::ResourcePath> paths) {
  for (const auto& path : paths) {
    AddResourcePath(path.path, path.id);
  }
}

void BraveWebUIIOSDataSource::SetDefaultResource(int resource_id) {
  default_resource_ = resource_id;
}

void BraveWebUIIOSDataSource::DisableDenyXFrameOptions() {
  deny_xframe_options_ = false;
}

const ui::TemplateReplacements* BraveWebUIIOSDataSource::GetReplacements()
    const {
  return &replacements_;
}

// URLDataSourceIOS implementation:

std::string BraveWebUIIOSDataSource::GetSource() const {
  return base::StrCat({kChromeUIScheme, url::kStandardSchemeSeparator});
}

void BraveWebUIIOSDataSource::StartDataRequest(
    const std::string& path,
    web::URLDataSourceIOS::GotDataCallback callback) {
  EnsureLoadTimeDataDefaultsAdded();

  if (use_strings_js_) {
    bool from_js_module = path == "strings.m.js";
    if (from_js_module || path == "strings.js") {
      SendLocalizedStringsAsJSON(std::move(callback), from_js_module);
      return;
    }
  }

  int resource_id = PathToIdrOrDefault(path);
  DCHECK_NE(resource_id, -1);
  scoped_refptr<base::RefCountedMemory> response(
      web::GetWebClient()->GetDataResourceBytes(resource_id));
  std::move(callback).Run(response);
}

std::string BraveWebUIIOSDataSource::GetMimeType(
    const std::string& path) const {
  if (base::EndsWith(path, ".png", base::CompareCase::INSENSITIVE_ASCII)) {
    return "image/png";
  }

  if (base::EndsWith(path, ".gif", base::CompareCase::INSENSITIVE_ASCII)) {
    return "image/gif";
  }

  if (base::EndsWith(path, ".jpg", base::CompareCase::INSENSITIVE_ASCII)) {
    return "image/jpg";
  }

  if (base::EndsWith(path, ".js", base::CompareCase::INSENSITIVE_ASCII)) {
    return "application/javascript";
  }

  if (base::EndsWith(path, ".json", base::CompareCase::INSENSITIVE_ASCII)) {
    return "application/json";
  }

  if (base::EndsWith(path, ".pdf", base::CompareCase::INSENSITIVE_ASCII)) {
    return "application/pdf";
  }

  if (base::EndsWith(path, ".css", base::CompareCase::INSENSITIVE_ASCII)) {
    return "text/css";
  }

  if (base::EndsWith(path, ".svg", base::CompareCase::INSENSITIVE_ASCII)) {
    return "image/svg+xml";
  }

  return "text/html";
}

bool BraveWebUIIOSDataSource::ShouldReplaceExistingSource() const {
  return replace_existing_source_;
}

bool BraveWebUIIOSDataSource::ShouldDenyXFrameOptions() const {
  return deny_xframe_options_;
}

bool BraveWebUIIOSDataSource::ShouldServiceRequest(const GURL& url) const {
  return web::GetWebClient()->IsAppSpecificURL(url);
}

void BraveWebUIIOSDataSource::EnsureLoadTimeDataDefaultsAdded() {
  if (load_time_data_defaults_added_) {
    return;
  }

  load_time_data_defaults_added_ = true;
  base::Value::Dict defaults;
  webui::SetLoadTimeDataDefaults(web::GetWebClient()->GetApplicationLocale(),
                                 &defaults);
  AddLocalizedStrings(defaults);
}

void BraveWebUIIOSDataSource::SendLocalizedStringsAsJSON(
    web::URLDataSourceIOS::GotDataCallback callback,
    bool from_js_module) {
  std::string template_data;
  webui::AppendJsonJS(localized_strings_, &template_data, from_js_module);
  std::move(callback).Run(
      base::MakeRefCounted<base::RefCountedString>(std::move(template_data)));
}

int BraveWebUIIOSDataSource::PathToIdrOrDefault(const std::string& path) const {
  auto it = path_to_idr_map_.find(path);
  return it == path_to_idr_map_.end() ? default_resource_ : it->second;
}

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
