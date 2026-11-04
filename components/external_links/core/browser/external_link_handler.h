/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_EXTERNAL_LINKS_CORE_BROWSER_EXTERNAL_LINK_HANDLER_H_
#define BRAVE_COMPONENTS_EXTERNAL_LINKS_CORE_BROWSER_EXTERNAL_LINK_HANDLER_H_

#include "base/memory/raw_ptr.h"
#include "brave/components/external_links/core/common/external_link_mode.h"

class GURL;
class HostContentSettingsMap;
class PrefRegistrySimple;
class PrefService;

namespace external_links {

// Reads and writes the default and per-site modes for links opened from other
// apps, and decides what to do with such a link.
class ExternalLinkHandler {
 public:
  ExternalLinkHandler(HostContentSettingsMap* settings_map, PrefService* prefs);
  ExternalLinkHandler(const ExternalLinkHandler&) = delete;
  ExternalLinkHandler& operator=(const ExternalLinkHandler&) = delete;
  ~ExternalLinkHandler();

  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

  ExternalLinkMode GetDefaultMode() const;
  void SetDefaultMode(ExternalLinkMode mode);

  // Returns the mode remembered for `url`'s site, falling back to the default.
  ExternalLinkMode GetMode(const GURL& url) const;

  // Remembers `mode` for `url`'s base domain (eTLD+1) and its subdomains.
  void SetSiteMode(const GURL& url, ExternalLinkMode mode);

  // Removes all remembered per-site modes, keeping the default.
  void ClearSiteModes();

  bool IsOnboardingShown() const;
  void SetOnboardingShown(bool shown);

  // A remembered site mode takes precedence over the default. Returns
  // kShowOnboarding at most once, marking onboarding as shown.
  ExternalLinkAction Resolve(const GURL& url);

 private:
  raw_ptr<HostContentSettingsMap> settings_map_;
  raw_ptr<PrefService> prefs_;
};

}  // namespace external_links

#endif  // BRAVE_COMPONENTS_EXTERNAL_LINKS_CORE_BROWSER_EXTERNAL_LINK_HANDLER_H_
