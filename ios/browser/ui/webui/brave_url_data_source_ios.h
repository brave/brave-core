// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_UI_WEBUI_BRAVE_URL_DATA_SOURCE_IOS_H_
#define BRAVE_IOS_BROWSER_UI_WEBUI_BRAVE_URL_DATA_SOURCE_IOS_H_

#include <string>

#include "ios/web/public/webui/url_data_source_ios.h"
#include "services/network/public/mojom/content_security_policy.mojom.h"

class BraveURLDataSourceIOS : public web::URLDataSourceIOS {
 public:
  BraveURLDataSourceIOS();
  ~BraveURLDataSourceIOS() override;

  virtual std::string GetContentSecurityPolicy(
      network::mojom::CSPDirectiveName directive) const;

 private:
  std::string GetContentSecurityPolicyBase() const override;
  std::string GetContentSecurityPolicyObjectSrc() const override;
  std::string GetContentSecurityPolicyFrameSrc() const override;

  // Brave CSP's & Security variables:
  base::flat_map<network::mojom::CSPDirectiveName, std::string> csp_overrides_;
};

#endif  // BRAVE_IOS_BROWSER_UI_WEBUI_BRAVE_URL_DATA_SOURCE_IOS_H_
