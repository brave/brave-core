// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/ui/webui/brave_webui_data_source.h"

#include "base/functional/bind.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/ref_counted_memory.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "ios/web/public/web_client.h"
#include "services/network/public/mojom/content_security_policy.mojom.h"
#include "ui/base/webui/jstemplate_builder.h"
#include "ui/base/webui/resource_path.h"
#include "ui/base/webui/web_ui_util.h"

namespace {
const char kChromeUIScheme[] = "chrome";
const char kChromeUIUntrustedScheme[] = "chrome-untrusted";
}  // namespace

BraveWebUIDataSource::BraveWebUIDataSource()
    : default_resource_(-1),
      use_strings_js_(false),
      deny_xframe_options_(true),
      load_time_data_defaults_added_(false),
      replace_existing_source_(true),
      should_replace_i18n_in_js_(false) {}

BraveWebUIDataSource::~BraveWebUIDataSource() = default;

void BraveWebUIDataSource::AddString(const std::string& name,
                                     const std::u16string& value) {
  localized_strings_.Set(name, value);
  replacements_[name] = base::UTF16ToUTF8(value);
}

void BraveWebUIDataSource::AddString(const std::string& name,
                                     const std::string& value) {
  localized_strings_.Set(name, value);
  replacements_[name] = value;
}

void BraveWebUIDataSource::AddLocalizedString(const std::string& name,
                                              int ids) {
  localized_strings_.Set(name, web::GetWebClient()->GetLocalizedString(ids));
  replacements_[name] =
      base::UTF16ToUTF8(web::GetWebClient()->GetLocalizedString(ids));
}

void BraveWebUIDataSource::AddLocalizedStrings(
    const base::Value::Dict& localized_strings) {
  localized_strings_.Merge(localized_strings.Clone());
  ui::TemplateReplacementsFromDictionaryValue(localized_strings,
                                              &replacements_);
}

void BraveWebUIDataSource::AddLocalizedStrings(
    base::span<const webui::LocalizedString> strings) {
  for (const auto& str : strings) {
    AddLocalizedString(str.name, str.id);
  }
}

void BraveWebUIDataSource::AddBoolean(const std::string& name, bool value) {
  localized_strings_.Set(name, value);
}

void BraveWebUIDataSource::UseStringsJs() {
  use_strings_js_ = true;
}

void BraveWebUIDataSource::EnableReplaceI18nInJS() {
  should_replace_i18n_in_js_ = true;
}

bool BraveWebUIDataSource::ShouldReplaceI18nInJS() const {
  return should_replace_i18n_in_js_;
}

void BraveWebUIDataSource::AddResourcePath(const std::string& path,
                                           int resource_id) {
  path_to_idr_map_[path] = resource_id;
}

void BraveWebUIDataSource::AddResourcePaths(
    base::span<const webui::ResourcePath> paths) {
  for (const auto& path : paths) {
    AddResourcePath(path.path, path.id);
  }
}

void BraveWebUIDataSource::SetDefaultResource(int resource_id) {
  default_resource_ = resource_id;
}

void BraveWebUIDataSource::DisableDenyXFrameOptions() {
  deny_xframe_options_ = false;
}

const ui::TemplateReplacements* BraveWebUIDataSource::GetReplacements() const {
  return &replacements_;
}

void BraveWebUIDataSource::OverrideContentSecurityPolicy(
    network::mojom::CSPDirectiveName directive,
    const std::string& value) {
  csp_overrides_.insert_or_assign(directive, value);
}

void BraveWebUIDataSource::AddFrameAncestor(const GURL& frame_ancestor) {
  // Do not allow a wildcard to be a frame ancestor or it will allow any website
  // to embed the WebUI.
  CHECK(frame_ancestor.SchemeIs(kChromeUIScheme) ||
        frame_ancestor.SchemeIs(kChromeUIUntrustedScheme));
  frame_ancestors_.insert(frame_ancestor);
}

void BraveWebUIDataSource::DisableTrustedTypesCSP() {
  // TODO(crbug.com/40137141): Trusted Type remaining WebUI
  // This removes require-trusted-types-for and trusted-types directives
  // from the CSP header.
  OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::RequireTrustedTypesFor, std::string());
  OverrideContentSecurityPolicy(network::mojom::CSPDirectiveName::TrustedTypes,
                                std::string());
}

// URLDataSourceIOS

std::string BraveWebUIDataSource::GetSource() const {
  return "chrome-untrusted://";
}

void BraveWebUIDataSource::StartDataRequest(const std::string& path,
                                            GotDataCallback callback) {
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

std::string BraveWebUIDataSource::GetMimeType(const std::string& path) const {
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

bool BraveWebUIDataSource::ShouldReplaceExistingSource() const {
  return replace_existing_source_;
}

bool BraveWebUIDataSource::AllowCaching() const {
  return false;
}

bool BraveWebUIDataSource::ShouldDenyXFrameOptions() const {
  return deny_xframe_options_;
}

bool BraveWebUIDataSource::ShouldServiceRequest(const GURL& url) const {
  return web::URLDataSourceIOS::ShouldServiceRequest(url);
}

std::string BraveWebUIDataSource::GetContentSecurityPolicy(
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
  return web::URLDataSourceIOS::GetContentSecurityPolicy(directive);
}

std::string BraveWebUIDataSource::GetContentSecurityPolicyObjectSrc() const {
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

  return web::URLDataSourceIOS::GetContentSecurityPolicyObjectSrc();
}

std::string BraveWebUIDataSource::GetContentSecurityPolicyFrameSrc() const {
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

void BraveWebUIDataSource::EnsureLoadTimeDataDefaultsAdded() {
  if (load_time_data_defaults_added_) {
    return;
  }

  load_time_data_defaults_added_ = true;
  base::Value::Dict defaults;
  webui::SetLoadTimeDataDefaults(web::GetWebClient()->GetApplicationLocale(),
                                 &defaults);
  AddLocalizedStrings(defaults);
}

void BraveWebUIDataSource::SendLocalizedStringsAsJSON(
    URLDataSourceIOS::GotDataCallback callback,
    bool from_js_module) {
  std::string template_data;
  webui::AppendJsonJS(localized_strings_, &template_data, from_js_module);
  std::move(callback).Run(
      base::MakeRefCounted<base::RefCountedString>(std::move(template_data)));
}

int BraveWebUIDataSource::PathToIdrOrDefault(const std::string& path) const {
  auto it = path_to_idr_map_.find(path);
  return it == path_to_idr_map_.end() ? default_resource_ : it->second;
}
