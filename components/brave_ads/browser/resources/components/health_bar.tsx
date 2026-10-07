/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Alert from '@brave/leo/react/alert'

import { useAppState } from '../lib/app_context'
import { computeHealthChecks, HealthCheck } from '../lib/health_checks'
import { useRoute, useRouter } from '../lib/router'

// Most severe first, so a real problem isn't buried below a pile of info-
// level notices.
const SEVERITY_ORDER: Record<HealthCheck['severity'], number> = {
  error: 0,
  warning: 1,
  info: 2,
}

// Rendered once at the top level (see `App` in app.tsx), independent of
// which tab is active, so a problem is visible no matter where the user
// navigates to rather than only on a Diagnostics visit.
export function HealthBar() {
  const state = useAppState((appState) => appState)
  const router = useRouter()
  const currentRoute = useRoute()

  const checks = React.useMemo(() => computeHealthChecks(state), [state])

  if (checks.length === 0) {
    return null
  }

  const sorted = [...checks].sort(
    (a, b) => SEVERITY_ORDER[a.severity] - SEVERITY_ORDER[b.severity])

  return (
    <div className='health-bar'>
      {sorted.map((check) => (
        <Alert key={check.id} type={check.severity}>
          {check.message}
          {check.route !== currentRoute && (
            <span
              slot='content-after'
              className='text-link'
              onClick={() => router.setRoute(check.route)}
            >
              View
            </span>
          )}
        </Alert>
      ))}
    </div>
  )
}
