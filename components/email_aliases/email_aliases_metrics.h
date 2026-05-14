/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_EMAIL_ALIASES_EMAIL_ALIASES_METRICS_H_
#define BRAVE_COMPONENTS_EMAIL_ALIASES_EMAIL_ALIASES_METRICS_H_

#include "base/memory/raw_ref.h"

class PrefRegistrySimple;
class PrefService;

namespace email_aliases {

inline constexpr char kSettingsPageMethodHistogramName[] =
    "Brave.EmailAliases.SettingsPageMethod";

enum class SettingsPageMethod {
  kContextMenu = 0,
  kAutofillBubble = 1,
  kAppMenu = 2,
  kManualNavigation = 3,
  kMaxValue = kManualNavigation,
};

class EmailAliasesMetrics {
 public:
  explicit EmailAliasesMetrics(PrefService& pref_service);
  ~EmailAliasesMetrics();

  EmailAliasesMetrics(const EmailAliasesMetrics&) = delete;
  EmailAliasesMetrics& operator=(const EmailAliasesMetrics&) = delete;

  static void RegisterPrefs(PrefRegistrySimple* registry);

  void RecordSettingsPageNavigation(SettingsPageMethod method);

 private:
  const raw_ref<PrefService> pref_service_;
};

}  // namespace email_aliases

#endif  // BRAVE_COMPONENTS_EMAIL_ALIASES_EMAIL_ALIASES_METRICS_H_
