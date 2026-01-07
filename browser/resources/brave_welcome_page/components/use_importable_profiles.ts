/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { useWelcomeApi } from '../api/welcome_api_context'
import { profileHasImportableTypes } from '../lib/import_profile_helper'

export function useImportableProfiles() {
  const api = useWelcomeApi()
  const profiles = api.useGetBrowserProfilesForImport().data

  return React.useMemo(() => {
    if (profiles) {
      return profiles.filter((profile) => {
        if (profile.name.startsWith('Bookmarks')) {
          return false
        }
        if (!profileHasImportableTypes(profile)) {
          return false
        }
        return true
      })
    }
    return null
  }, [profiles])
}
