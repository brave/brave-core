/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DIAGNOSTICS_DIAGNOSTIC_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DIAGNOSTICS_DIAGNOSTIC_MANAGER_H_

#include <memory>

#include "brave/components/brave_ads/core/internal/diagnostics/diagnostic_alias.h"
#include "brave/components/brave_ads/core/public/ads_callback.h"

namespace brave_ads {

class DiagnosticManager final {
 public:
  DiagnosticManager();

  DiagnosticManager(const DiagnosticManager&) = delete;
  DiagnosticManager& operator=(const DiagnosticManager&) = delete;

  ~DiagnosticManager();

  static DiagnosticManager& GetInstance();

  void SetEntry(std::unique_ptr<DiagnosticEntryInterface> entry);

  void GetDiagnostics(GetDiagnosticsCallback callback) const;

 private:
  DiagnosticMap diagnostics_;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DIAGNOSTICS_DIAGNOSTIC_MANAGER_H_
