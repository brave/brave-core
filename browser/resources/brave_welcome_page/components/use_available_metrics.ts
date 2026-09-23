/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { useWelcomeApi } from '../api/welcome_api_context'

interface AvailableMetrics {
  webDiscovery: boolean
  p3a: boolean
  crashReports: boolean
}

// Returns information about the metrics that can be shown to the user on the
// metrics step.
export function useAvailableMetrics(): AvailableMetrics {
  const api = useWelcomeApi()

  const webDiscoveryFeatureEnabled = api.useWebDiscoveryFeatureEnabledData()
  const isWebDiscoveryPrefManaged = api.useIsWebDiscoveryPrefManagedData()
  const isP3APrefManaged = api.useIsP3APrefManagedData()
  const isCrashReportingPrefManaged = api.useIsCrashReportingPrefManagedData()

  return {
    webDiscovery: webDiscoveryFeatureEnabled && !isWebDiscoveryPrefManaged,
    p3a: !isP3APrefManaged,
    crashReports: !isCrashReportingPrefManaged,
  }
}

// Returns whether any item can be displayed on the metrics step.
export function hasMetricsAvailable(metrics: AvailableMetrics) {
  return Object.values(metrics).some((available) => available)
}
