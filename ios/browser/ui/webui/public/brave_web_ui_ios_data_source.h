// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_UI_WEBUI_PUBLIC_BRAVE_WEB_UI_IOS_DATA_SOURCE_H_
#define BRAVE_IOS_BROWSER_UI_WEBUI_PUBLIC_BRAVE_WEB_UI_IOS_DATA_SOURCE_H_

#include <map>
#include <set>
#include <string>

#include "base/containers/flat_map.h"
#include "ios/web/public/webui/url_data_source_ios.h"
#include "ios/web/public/webui/web_ui_ios_data_source.h"
#include "ios/web/webui/url_data_source_ios_impl.h"
#include "services/network/public/mojom/content_security_policy.mojom.h"

namespace webui {
struct LocalizedString;
struct ResourcePath;
}  // namespace webui

class BraveWebUIIOSDataSource : public web::URLDataSourceIOSImpl,
                                public web::WebUIIOSDataSource {
 public:
  static web::WebUIIOSDataSource* Create(const std::string& source_name);

  BraveWebUIIOSDataSource(const BraveWebUIIOSDataSource&) = delete;
  BraveWebUIIOSDataSource& operator=(const BraveWebUIIOSDataSource&) = delete;

  // WebUIIOSDataSource implementation:
  void AddString(const std::string& name, const std::u16string& value) override;
  void AddString(const std::string& name, const std::string& value) override;
  void AddLocalizedString(const std::string& name, int ids) override;
  void AddLocalizedStrings(const base::Value::Dict& localized_strings) override;
  void AddLocalizedStrings(
      base::span<const webui::LocalizedString> strings) override;
  void AddBoolean(const std::string& name, bool value) override;
  void UseStringsJs() override;
  void EnableReplaceI18nInJS() override;
  bool ShouldReplaceI18nInJS() const override;
  void AddResourcePath(const std::string& path, int resource_id) override;
  void AddResourcePaths(base::span<const webui::ResourcePath> paths) override;
  void SetDefaultResource(int resource_id) override;
  void DisableDenyXFrameOptions() override;
  const ui::TemplateReplacements* GetReplacements() const override;

  // Brave CSP's & Security implementation:
  virtual void OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName directive,
      const std::string& value);

  virtual void AddFrameAncestor(const GURL& frame_ancestor);
  virtual void DisableTrustedTypesCSP();

 protected:
  ~BraveWebUIIOSDataSource() override;

  // URLDataSourceIOS overrides:
  virtual std::string GetSource() const;
  virtual void StartDataRequest(
      const std::string& path,
      web::URLDataSourceIOS::GotDataCallback callback);
  virtual std::string GetMimeType(const std::string& path) const;
  virtual bool ShouldReplaceExistingSource() const;
  virtual bool ShouldDenyXFrameOptions() const;
  virtual bool ShouldServiceRequest(const GURL& url) const;

 private:
  class InternalDataSource;
  friend class InternalDataSource;

  explicit BraveWebUIIOSDataSource(const std::string& source_name);

  // URLDataSourceIOS implementation:
  void EnsureLoadTimeDataDefaultsAdded();
  void SendLocalizedStringsAsJSON(
      web::URLDataSourceIOS::GotDataCallback callback,
      bool from_js_module);
  int PathToIdrOrDefault(const std::string& path) const;

  // WebUIIOSDataSource variables:
  int default_resource_;
  bool use_strings_js_;
  std::map<std::string, int> path_to_idr_map_;
  base::Value::Dict localized_strings_;
  ui::TemplateReplacements replacements_;
  bool deny_xframe_options_;
  bool load_time_data_defaults_added_;
  bool replace_existing_source_;
  bool should_replace_i18n_in_js_;
  std::optional<std::string> supported_scheme_;

  // Brave CSP's & Security variables:
  base::flat_map<network::mojom::CSPDirectiveName, std::string> csp_overrides_;
  std::set<GURL> frame_ancestors_;
};

#endif  // BRAVE_IOS_BROWSER_UI_WEBUI_PUBLIC_BRAVE_WEB_UI_IOS_DATA_SOURCE_H_
