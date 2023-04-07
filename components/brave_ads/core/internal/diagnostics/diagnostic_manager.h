/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DIAGNOSTICS_DIAGNOSTIC_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DIAGNOSTICS_DIAGNOSTIC_MANAGER_H_

#include <memory>

#include "brave/components/brave_ads/core/ads_callback.h"
#include "brave/components/brave_ads/core/internal/diagnostics/diagnostic_alias.h"

namespace brave_ads {

class DiagnosticManager final {
 public:
  DiagnosticManager();

  DiagnosticManager(const DiagnosticManager& other) = delete;
  DiagnosticManager& operator=(const DiagnosticManager& other) = delete;

  DiagnosticManager(DiagnosticManager&& other) noexcept = delete;
  DiagnosticManager& operator=(DiagnosticManager&& other) noexcept = delete;

  ~DiagnosticManager();

  static DiagnosticManager* GetInstance();

  static bool HasInstance();

  void SetEntry(std::unique_ptr<DiagnosticEntryInterface> entry);

  void GetDiagnostics(GetDiagnosticsCallback callback) const;

 private:
  DiagnosticMap diagnostics_;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DIAGNOSTICS_DIAGNOSTIC_MANAGER_H_
