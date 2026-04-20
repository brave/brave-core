// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as Mojom from 'gen/brave/components/psst/common/psst_ui_common.mojom.m.js'

import { createPsstDialogApi, type PsstDialogAPI } from './psst_dialog_api'

/**
 * Mock implementations for PSST Dialog API used in Storybook stories and tests.
 *
 * @example
 * // Basic usage with defaults
 * const mockAPI = createMockPsstDialogAPI()
 *
 * // With custom state
 * const mockAPI = createMockPsstDialogAPI({
 *   settingsCardData: {
 *     siteName: 'custom-site.com',
 *     items: customSettingItems
 *   }
 * })
 */

/**
 * Mock setting card data with realistic privacy settings
 */
const mockSettingCardData: Mojom.SettingCardData = {
  siteName: 'example.com',
  items: [
    {
      uid: '1',
      url: 'https://example.com/cookies',
      description: 'Manage cookie preferences and tracking protection',
    },
    {
      uid: '2',
      url: 'https://example.com/analytics',
      description: 'Control analytics and data collection settings',
    },
    {
      uid: '3',
      url: 'https://example.com/ads',
      description: 'Configure advertising preferences',
    },
    {
      uid: '4',
      url: 'https://example.com/location',
      description: 'Manage location data sharing',
    },
  ],
}

/**
 * Mock Mojom interfaces for testing
 */
function createMockConsentHelper(): Mojom.PsstConsentHelperInterface {
  const mock = {
    // Mojom interface methods that endpointsFor/actionsFor expect
    async performPrivacyTuning(performForUids: string[]) {
      console.log('[Mock] performPrivacyTuning called with:', {
        performForUids,
      })
    },

    async closeDialog() {
      console.log('[Mock] closeDialog called')
    },

    // Mock Mojom remote properties (these are usually
    // auto-generated)
    onConnectionError: { addListener: () => {} } as any,
    $: {
      bindNewPipeAndPassReceiver: () => ({}),
    } as any,
  } as Mojom.PsstConsentHelperInterface

  return mock
}

export interface MockPsstDialogAPIOptions {
  /**
   * Initial settings card data to display
   */
  settingsCardData?: Partial<Mojom.SettingCardData>

  /**
   * Simulate request processing delays (in ms)
   */
  requestDelay?: number

  /**
   * Simulate errors for specific UIDs
   */
  errorUids?: string[]

  /**
   * Custom close dialog behavior
   */
  onCloseDialog?: () => void
}

/**
 * Creates a mock PSST Dialog API for Storybook and testing.
 *
 * @param options Configuration options for the mock API
 * @returns Mock PsstDialogAPI with controllable behavior
 */
export function createMockPsstDialogAPI(
  options: MockPsstDialogAPIOptions = {},
): PsstDialogAPI {
  const {
    settingsCardData = {},
    requestDelay = 1000,
    errorUids = [],
    onCloseDialog = () => console.log('[Mock] Dialog closed'),
  } = options

  // Merge provided settings with defaults
  const finalSettingsData: Mojom.SettingCardData = {
    ...mockSettingCardData,
    ...settingsCardData,
    items: settingsCardData.items || mockSettingCardData.items,
  }

  const mockConsentHelper = createMockConsentHelper()

  const { api, dialogHandler } = createPsstDialogApi(mockConsentHelper)

  console.log('[Mock] Creating API with:', {
    mockConsentHelper,
    dialogHandler,
  })

  // Override the close dialog action
  mockConsentHelper.closeDialog = async () => {
    onCloseDialog()
  }

  // Override the perform privacy tuning action with mock behavior
  mockConsentHelper.performPrivacyTuning = async (performForUids: string[]) => {
    console.log('[Mock] Processing perform privacy tuning for:', {
      performForUids,
    })

    // For mocking, assume all items are being processed (in real scenario,
    // only checked items would be sent to this method)
    const allUids = finalSettingsData.items.map((item) => item.uid)

    // Simulate processing each URL
    for (const item of finalSettingsData.items) {
      if (!performForUids.includes(item.uid)) {
        continue
      }
      // Simulate request status update
      setTimeout(() => {
        const hasError = errorUids.includes(item.uid)
        console.log(
          `[Mock] Simulate request done for URL: ${item.uid} with error: ${hasError ? 'Failed to update setting' : 'Success'}`,
        )
        dialogHandler.onSetRequestStatus(
          item.uid,
          hasError ? 'Failed to update setting' : null,
        )
      }, Math.random() * requestDelay)
    }
  }
  console.log('[Mock] Created API:', api)

  if (!api) {
    console.error('[Mock] API creation failed!')
    throw new Error('Failed to create mock API')
  }

  const initialData: Mojom.SettingCardData = finalSettingsData
  return {
    api,
    dialogHandler,
    initialData,
  }
}
