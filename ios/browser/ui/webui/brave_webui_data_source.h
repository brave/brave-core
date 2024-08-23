// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_UI_WEBUI_BRAVE_WEBUI_DATA_SOURCE_H_
#define BRAVE_IOS_BROWSER_UI_WEBUI_BRAVE_WEBUI_DATA_SOURCE_H_

#include <cstdint>
#include <map>
#include <set>
#include <string>

#include "base/functional/callback.h"
#include "base/values.h"
#include "ios/web/public/webui/url_data_source_ios.h"
#include "ios/web/webui/url_data_manager_ios.h"
#include "ui/base/template_expressions.h"

namespace network::mojom {
enum class CSPDirectiveName : std::int32_t;
}  // namespace network::mojom

namespace webui {
struct LocalizedString;
struct ResourcePath;
}  // namespace webui

class BraveWebUIDataSource : public web::URLDataSourceIOS {
 public:
  BraveWebUIDataSource();

  ~BraveWebUIDataSource() override;

  BraveWebUIDataSource(const BraveWebUIDataSource&) = delete;
  BraveWebUIDataSource& operator=(const BraveWebUIDataSource&) = delete;

  void AddString(const std::string& name, const std::u16string& value);
  void AddString(const std::string& name, const std::string& value);
  void AddLocalizedString(const std::string& name, int ids);
  void AddLocalizedStrings(const base::Value::Dict& localized_strings);
  void AddLocalizedStrings(base::span<const webui::LocalizedString> strings);
  void AddBoolean(const std::string& name, bool value);
  void UseStringsJs();
  void EnableReplaceI18nInJS();
  void AddResourcePath(const std::string& path, int resource_id);
  void AddResourcePaths(base::span<const webui::ResourcePath> paths);
  void SetDefaultResource(int resource_id);
  void DisableDenyXFrameOptions();
  const ui::TemplateReplacements* GetReplacements() const;

  void OverrideContentSecurityPolicy(network::mojom::CSPDirectiveName directive,
                                     const std::string& value);
  void AddFrameAncestor(const GURL& frame_ancestor);
  void DisableTrustedTypesCSP();

 private:
  void EnsureLoadTimeDataDefaultsAdded();
  void SendLocalizedStringsAsJSON(GotDataCallback callback,
                                  bool from_js_module);
  int PathToIdrOrDefault(const std::string& path) const;

  // web::URLDataSourceIOS overrides:
  std::string GetSource() const override;
  void StartDataRequest(const std::string& path,
                        GotDataCallback callback) override;
  std::string GetMimeType(const std::string& path) const override;
  bool ShouldReplaceExistingSource() const override;
  bool ShouldReplaceI18nInJS() const override;
  bool AllowCaching() const override;
  bool ShouldDenyXFrameOptions() const override;

  bool ShouldServiceRequest(const GURL& url) const override;
  std::string GetContentSecurityPolicy(
      network::mojom::CSPDirectiveName directive) const override;
  std::string GetContentSecurityPolicyObjectSrc() const override;
  std::string GetContentSecurityPolicyFrameSrc() const override;

  int default_resource_;
  bool use_strings_js_;
  std::map<std::string, int> path_to_idr_map_;
  base::Value::Dict localized_strings_;
  ui::TemplateReplacements replacements_;
  bool deny_xframe_options_;
  bool load_time_data_defaults_added_;
  bool replace_existing_source_;
  bool should_replace_i18n_in_js_;

  base::flat_map<network::mojom::CSPDirectiveName, std::string> csp_overrides_;
  std::set<GURL> frame_ancestors_;
};

#endif  // BRAVE_IOS_BROWSER_UI_WEBUI_BRAVE_WEBUI_DATA_SOURCE_H_
