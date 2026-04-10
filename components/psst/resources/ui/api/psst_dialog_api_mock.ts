// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { createPsstDialogApi, type PsstDialogAPI } from './psst_dialog_api'
import * as Mojom from './psst_dialog_api'

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
 *     site_name: 'custom-site.com',
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
function createMockConsentHelper(): Mojom.PsstConsentHelperRemote {
  const mock = {
    // Mojom interface methods that endpointsFor/actionsFor expect
    async applyChanges(params: [string, string[]]) {
      const [siteName, disabledSettingsList] = params
      console.log('[Mock] applyChanges called with:', { siteName, disabledSettingsList })
      return Promise.resolve()
    },
    
    async closeDialog() {
      console.log('[Mock] closeDialog called')
      return Promise.resolve()
    },
    
    // Mock Mojom remote properties (these are usually auto-generated)
    $: {
      bindNewPipeAndPassReceiver: () => ({}),
    } as any,
  }
  
  return mock as Mojom.PsstConsentHelperRemote
}

function createMockCallbackRouter(): Mojom.PsstConsentDialogCallbackRouter {
  // Mock listeners that can be called to simulate state updates
  const listeners = {
    setSettingsCardData: [] as Array<(data: Mojom.SettingCardData) => void>,
    onSetRequestDone: [] as Array<(url: string, error?: string) => void>,
    onSetCompleted: [] as Array<(appliedChecks?: string[], errors?: string[]) => void>,
  }

  const mock = {
    // Callback router interface
    setSettingsCardData: {
      addListener: (callback: (data: Mojom.SettingCardData) => void) => {
        listeners.setSettingsCardData.push(callback)
      },
    },
    onSetRequestDone: {
      addListener: (callback: (url: string, error?: string) => void) => {
        listeners.onSetRequestDone.push(callback)
      },
    },
    onSetCompleted: {
      addListener: (callback: (appliedChecks?: string[], errors?: string[]) => void) => {
        listeners.onSetCompleted.push(callback)
      },
    },
    
    // Methods to trigger listeners (for testing scenarios)
    _triggerSettingsCardDataUpdate: (data: Mojom.SettingCardData) => {
      listeners.setSettingsCardData.forEach(callback => callback(data))
    },
    _triggerRequestDone: (url: string, error?: string) => {
      listeners.onSetRequestDone.forEach(callback => callback(url, error))
    },
    _triggerCompleted: (appliedChecks?: string[], errors?: string[]) => {
      listeners.onSetCompleted.forEach(callback => callback(appliedChecks, errors))
    },
    
    // Mock Mojom remote properties
    $: {
      bindNewPipeAndPassRemote: () => ({}),
    } as any,
  }
  
  return mock as any
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
  options: MockPsstDialogAPIOptions = {}
): PsstDialogAPI & { _simulateActions: any } {
  const {
    settingsCardData = {},
    autoLoadSettings = true,
    requestDelay = 1000,
    errorUrls = [],
    onCloseDialog = () => console.log('[Mock] Dialog closed')
  } = options

  // Merge provided settings with defaults
  const finalSettingsData: Mojom.SettingCardData = {
    ...mockSettingCardData,
    ...settingsCardData,
    items: settingsCardData.items || mockSettingCardData.items
  }

  const mockConsentHelper = createMockConsentHelper()
  const mockCallbackRouter = createMockCallbackRouter()

  console.log('[Mock] Creating API with:', { mockConsentHelper, mockCallbackRouter })

  // Override the close dialog action
  mockConsentHelper.closeDialog = async () => {
    onCloseDialog()
  }

  // Override the apply changes action with mock behavior
  mockConsentHelper.applyChanges = async (params: [string, string[]]) => {
    const [siteName, disabledSettingsList] = params
    console.log('[Mock] Processing apply changes for:', { siteName, disabledSettingsList })
    
    // For mocking, assume all items are being processed (in real scenario, 
    // only checked items would be sent to this method)
    const allUrls = finalSettingsData.items.map(item => item.url)

    // Simulate processing each URL
    for (const item of finalSettingsData.items) {
      // Simulate request status update
      setTimeout(() => {
        const hasError = errorUrls.includes(item.url)
        ;(mockCallbackRouter as any)._triggerRequestDone(
          item.url,
          hasError ? 'Failed to update setting' : undefined
        )
      }, Math.random() * requestDelay)
    }

    // Simulate completion
    setTimeout(() => {
      const appliedChecks = allUrls.filter(url => !errorUrls.includes(url))
      const errors = errorUrls.filter(url => allUrls.includes(url))
      ;(mockCallbackRouter as any)._triggerCompleted(appliedChecks, errors.length > 0 ? errors : undefined)
    }, requestDelay + 500)
  }

  const api = createPsstDialogApi(mockConsentHelper, mockCallbackRouter)
  console.log('[Mock] Created API:', api)

  if (!api) {
    console.error('[Mock] API creation failed!')
    throw new Error('Failed to create mock API')
  }

  // Auto-load settings if enabled
  if (autoLoadSettings) {
    setTimeout(() => {
      ;(mockCallbackRouter as any)._triggerSettingsCardDataUpdate(finalSettingsData)
    }, 100)
  }

  // Add simulation methods to the returned API for testing
  ;(api as any)._simulateActions = {
    /**
     * Simulate receiving new settings data
     */
    updateSettingsData: (data: Partial<Mojom.SettingCardData>) => {
      const updatedData = { ...finalSettingsData, ...data }
      ;(mockCallbackRouter as any)._triggerSettingsCardDataUpdate(updatedData)
    },

    /**
     * Simulate a request completion for a specific URL
     */
    triggerRequestDone: (url: string, error?: string) => {
      ;(mockCallbackRouter as any)._triggerRequestDone(url, error)
    },

    /**
     * Simulate overall completion with results
     */
    triggerCompleted: (appliedChecks?: string[], errors?: string[]) => {
      ;(mockCallbackRouter as any)._triggerCompleted(appliedChecks, errors)
    },

    /**
     * Get current mock router for advanced testing scenarios
     */
    getCallbackRouter: () => mockCallbackRouter,
  }

  return api
}
