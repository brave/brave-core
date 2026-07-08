/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { WelcomeApi, createWelcomeApi, ImportDataStatus } from './welcome_api'

export function createWelcomeApiMock(): WelcomeApi {
  let onImportStatusChange: ((status: ImportDataStatus) => void) | null = null

  const api = createWelcomeApi({
    welcomePageHandler: {
      setWelcomePage(page) {},
    },
    messages: {
      async getDefaultBrowserInfo() {
        return {
          canBeDefault: true,
          isDefault: false,
          isDisabledByPolicy: false,
          isUnknownError: false,
        }
      },
      async getBrowserProfilesForImport() {
        return [
          {
            index: 0,
            name: 'Google Chrome Person 1',
            profileName: '',
            autofillFormData: true,
            extensions: true,
            favorites: true,
            history: true,
            passwords: true,
            search: true,
          },
          {
            index: 1,
            name: 'Google Chrome Person 2',
            profileName: '',
            autofillFormData: true,
            extensions: true,
            favorites: true,
            history: true,
            passwords: true,
            search: true,
          },
          {
            index: 2,
            name: 'Microsoft Edge',
            profileName: '',
            autofillFormData: true,
            extensions: true,
            favorites: true,
            history: true,
            passwords: true,
            search: true,
          },
        ]
      },
      setAsDefaultBrowser() {},
      importData(profileIndex, types) {
        if (!onImportStatusChange) {
          return
        }
        onImportStatusChange('in-progress')
        setTimeout(() => {
          if (onImportStatusChange) {
            onImportStatusChange('succeeded')
          }
        }, 2000)
      },
      addImportStatusListener(fn) {
        onImportStatusChange = fn
      },
    },
  })

  return api
}
