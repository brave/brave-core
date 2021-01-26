/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/values.h"
#include "brave/components/unstoppable_domains/buildflags/buildflags.h"

#if BUILDFLAG(UNSTOPPABLE_DOMAINS_ENABLED)
#include "brave/components/unstoppable_domains/constants.h"
#endif

namespace settings {
namespace {
std::unique_ptr<base::DictionaryValue> CreateSecureDnsSettingDict();
}  // namespace
}  // namespace settings

#include "../../../../../../../chrome/browser/ui/webui/settings/settings_secure_dns_handler.cc"

namespace settings {

namespace {

// Hide Unstoppable Domains resolver in the custom provider list in settings
// because it will be used for name resolution only for TLDs from Unstoppable
// Domains, instaed of a global DoH settings.
std::unique_ptr<base::DictionaryValue> CreateSecureDnsSettingDict() {
  auto dict = CreateSecureDnsSettingDict_ChromiumImpl();
#if BUILDFLAG(UNSTOPPABLE_DOMAINS_ENABLED)
  if (!dict->FindListPath("templates")) {
    return dict;
  }

  auto secure_dns_templates = std::make_unique<base::ListValue>();
  for (const auto& template_str : dict->FindListPath("templates")->GetList()) {
    if (!template_str.is_string())
      return dict;
    if (template_str.GetString() != unstoppable_domains::kDoHResolver)
      secure_dns_templates->Append(template_str.GetString());
  }

  dict->SetList("templates", std::move(secure_dns_templates));
#endif  // BUILDFLAG(UNSTOPPABLE_DOMAINS_ENABLED)
  return dict;
}

}  // namespace

}  // namespace settings
