// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_NEW_TAB_PAGE_BRAVE_UNTRUSTED_SOURCE_H_
#define BRAVE_BROWSER_UI_WEBUI_NEW_TAB_PAGE_BRAVE_UNTRUSTED_SOURCE_H_

#include <string>

#include "chrome/browser/ui/webui/new_tab_page/untrusted_source.h"

// For special handling from brave://newtab.
class BraveUntrustedSource : public UntrustedSource {
 public:
  using UntrustedSource::UntrustedSource;
  ~BraveUntrustedSource() override;
  BraveUntrustedSource(const BraveUntrustedSource&) = delete;
  BraveUntrustedSource& operator=(const BraveUntrustedSource&) = delete;

  // UntrustedSource overrides:
  std::string GetContentSecurityPolicy(
      network::mojom::CSPDirectiveName directive) override;
  void StartDataRequest(
      const GURL& url,
      const content::WebContents::Getter& wc_getter,
      content::URLDataSource::GotDataCallback callback) override;
  bool ShouldServiceRequest(const GURL& url,
                            content::BrowserContext* browser_context,
                            int render_process_id) override;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_NEW_TAB_PAGE_BRAVE_UNTRUSTED_SOURCE_H_
