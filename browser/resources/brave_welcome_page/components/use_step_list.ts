/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { useImportableProfiles } from './use_importable_profiles'

export type Step = 'welcome' | 'import' | 'appearance' | 'features' | 'metrics'

const baseSteps: Step[] = [
  'welcome',
  'import',
  'appearance',
  'features',
  'metrics',
]

export function useStepList() {
  const profiles = useImportableProfiles()

  return React.useMemo(() => {
    if (!profiles) {
      return []
    }

    const hidden = new Set<Step>()

    if (profiles.length === 0) {
      hidden.add('import')
    }

    return baseSteps.filter((step) => !hidden.has(step))
  }, [profiles])
}
