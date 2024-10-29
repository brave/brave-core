// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_IOS_WEB_WEBUI_WEB_UI_IOS_DATA_SOURCE_IMPL_H_
#define BRAVE_CHROMIUM_SRC_IOS_WEB_WEBUI_WEB_UI_IOS_DATA_SOURCE_IMPL_H_

#include <map>
#include <set>
#include <string>

#include "base/containers/flat_map.h"
#include "ios/web/public/webui/web_ui_ios_data_source.h"
#include "services/network/public/mojom/content_security_policy.mojom.h"

class BraveWebUIIOSDataSource;

#define should_replace_i18n_in_js_ \
  should_replace_i18n_in_js_;      \
  friend BraveWebUIIOSDataSource
#include "src/ios/web/webui/web_ui_ios_data_source_impl.h"  // IWYU pragma: export
#undef should_replace_i18n_in_js_

class BraveWebUIIOSDataSource : public web::WebUIIOSDataSourceImpl {
 public:
  static web::WebUIIOSDataSource* Create(const std::string& source_name);

  // Brave CSP's & Security implementation:
  virtual void OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName directive,
      const std::string& value);

  virtual void AddFrameAncestor(const GURL& frame_ancestor);
  virtual void DisableTrustedTypesCSP();

 protected:
  ~BraveWebUIIOSDataSource() override;

 private:
  class InternalDataSource;
  friend class InternalDataSource;
  friend class WebUIIOSDataSourceImpl;
  explicit BraveWebUIIOSDataSource(const std::string& source_name);

  // Brave CSP's & Security variables:
  base::flat_map<network::mojom::CSPDirectiveName, std::string> csp_overrides_;
  std::set<GURL> frame_ancestors_;
};

#endif  // BRAVE_CHROMIUM_SRC_IOS_WEB_WEBUI_WEB_UI_IOS_DATA_SOURCE_IMPL_H_
