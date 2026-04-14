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
 *     setting_items: customSettingItems
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
      url: 'https://example.com/cookies',
      description: 'Manage cookie preferences and tracking protection',
    },
    {
      url: 'https://example.com/analytics',
      description: 'Control analytics and data collection settings',
    },
    {
      url: 'https://example.com/ads',
      description: 'Configure advertising preferences',
    },
    {
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
    async applyChanges(siteName: string, disabledSettingsList: string[]) {
      console.log('[Mock] applyChanges called with:', {
        siteName,
        disabledSettingsList,
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
   * Whether to auto-trigger setting data on mount
   */
  autoLoadSettings?: boolean

  /**
   * Simulate request processing delays (in ms)
   */
  requestDelay?: number

  /**
   * Simulate errors for specific URLs
   */
  errorUrls?: string[]

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
): PsstDialogAPI & {
  api: PsstDialogAPI['api'] & { _simulateActions: any }
} {
  const {
    settingsCardData = {},
    autoLoadSettings = true,
    requestDelay = 1000,
    errorUrls = [],
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

  // Override the apply changes action with mock behavior
  mockConsentHelper.applyChanges = async (
    siteName: string,
    disabledSettingsList: string[],
  ) => {
    console.log('[Mock] Processing apply changes for:', {
      siteName,
      disabledSettingsList,
    })

    // For mocking, assume all items are being processed (in real scenario,
    // only checked items would be sent to this method)
    const allUrls = finalSettingsData.items.map((item) => item.url)

    // Simulate processing each URL
    for (const item of finalSettingsData.items) {
      // Simulate request status update
      setTimeout(() => {
        const hasError = errorUrls.includes(item.url)
        console.log(
          `[Mock] Simulate request done for URL: ${item.url} with error: ${hasError ? 'Failed to update setting' : 'Success'}`,
        )
        dialogHandler.onSetRequestDone(
          item.url,
          hasError ? 'Failed to update setting' : null,
        )
      }, Math.random() * requestDelay)
    }

    // Simulate completion
    setTimeout(() => {
      const appliedChecks = allUrls.filter((url) => !errorUrls.includes(url))
      const errors = errorUrls.filter((url) => allUrls.includes(url))
      console.log(
        `[Mock] Simulate completion with applied checks: ${appliedChecks} and errors: ${errors}`,
      )
      dialogHandler.onSetCompleted(appliedChecks, errors)
    }, requestDelay + 500)
  }
  console.log('[Mock] Created API:', api)

  if (!api) {
    console.error('[Mock] API creation failed!')
    throw new Error('Failed to create mock API')
  }

  // Auto-load settings if enabled
  if (autoLoadSettings) {
    setTimeout(() => {
      console.log('[Mock] Auto-loading settings data:', finalSettingsData)
      dialogHandler.setSettingsCardData(finalSettingsData)
    }, 100)
  }

  return {
    api,
    dialogHandler,
  } as PsstDialogAPI & {
    api: PsstDialogAPI['api'] & { _simulateActions: any }
  }
}
