/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { useImportableProfiles } from './use_importable_profiles'

const baseSteps = ['welcome', 'import', 'appearance'] as const

export type Step = (typeof baseSteps)[number]

// Returns the current list of visible Welcome steps.
export function useStepList() {
  const profiles = useImportableProfiles()

  return React.useMemo(() => {
    const hidden = new Set<Step>()

    // Hide the import step if there are no importable profiles.
    if (!profiles?.length) {
      hidden.add('import')
    }

    return baseSteps.filter((step) => !hidden.has(step))
  }, [profiles])
}
