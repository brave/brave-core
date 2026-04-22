/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { Meta, StoryObj } from '@storybook/react'

import * as Mojom from 'gen/brave/components/psst/common/psst_ui_common.mojom.m.js'
import Flex from '$web-common/Flex'
import { PsstDialogAPIProvider } from './api/psst_dialog_api_context'
import { PsstProgressModal } from './components/PsstProgressModal'
import { createPsstDialogApi } from './api/psst_dialog_api'

// Stable default values to prevent unnecessary re-renders
const DEFAULT_ERROR_UIDS: string[] = []
const DEFAULT_ITEMS: Mojom.SettingCardDataItem[] = []

function PsstDialogStory({
  requestDelay = 1000,
  errorUids = DEFAULT_ERROR_UIDS,
  siteName = 'example.com',
  items = DEFAULT_ITEMS,
}: {
  readonly requestDelay?: number
  readonly errorUids?: string[]
  readonly siteName?: string
  readonly items?: Mojom.SettingCardDataItem[]
}) {
  const mockAPI = React.useMemo(() => {
    const result = createStorybookAPI({
      requestDelay,
      errorUids,
      settingsCardData: {
        siteName,
        items,
      },
      onReportFailedContent: () =>
        console.log('[Storybook] Report failed content'),
      onCloseDialog: () => console.log('[Storybook] Dialog closed'),
    })
    return {
      api: result.api,
      dialogHandler: result.dialogHandler,
      siteData: result.siteData,
    }
  }, [requestDelay, errorUids, siteName, items])

  return (
    <Flex
      direction='row'
      justify='center'
      align='flex-start'
    >
      <PsstDialogAPIProvider
        api={mockAPI.api}
        siteData={mockAPI.siteData}
      >
        <PsstProgressModal />
      </PsstDialogAPIProvider>
    </Flex>
  )
}

export default {
  title: 'PSST/Privacy Settings Dialog',
  component: PsstDialogStory,
  parameters: {
    layout: 'fullscreen',
    docs: {
      description: {
        component:
          'Privacy Settings Synchronization Tool (PSST) dialog for managing site privacy preferences.',
      },
    },
  },
  argTypes: {
    siteName: {
      control: 'text',
      description: 'Name of the site to configure privacy settings for',
    },
    requestDelay: {
      control: { type: 'range', min: 100, max: 5000, step: 100 },
      description: 'Simulated network delay in milliseconds',
    },
    errorUids: {
      control: 'object',
      description: 'Array of UIDs that should simulate errors',
    },
  },
} satisfies Meta<typeof PsstDialogStory>

type Story = StoryObj<typeof PsstDialogStory>

/**
 * Default dialog state with typical privacy settings loaded
 */
export const Default: Story = {
  args: {
    siteName: 'example.com',
    requestDelay: 1000,
    items: [
      {
        uid: '1',
        description: 'Manage cookie preferences and tracking protection',
      },
      {
        uid: '2',
        description: 'Control analytics and data collection settings',
      },
      {
        uid: '3',
        description: 'Configure advertising preferences',
      },
      {
        uid: '4',
        description: 'Manage location data sharing',
      },
    ],
    errorUids: [],
  },
}

/**
 * Dialog with some failing requests to show error handling
 */
export const WithErrors: Story = {
  args: {
    siteName: 'example.com',
    requestDelay: 1500,
    items: [
      {
        uid: '1',
        description: 'Manage cookie preferences and tracking protection',
      },
      {
        uid: '2',
        description: 'Control analytics and data collection settings',
      },
      {
        uid: '3',
        description: 'Configure advertising preferences',
      },
      {
        uid: '4',
        description: 'Manage location data sharing',
      },
    ],
    errorUids: ['2', '4'],
  },
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
   * Custom report failed content behavior
   */
  onReportFailedContent: () => void

  /**
   * Custom close dialog behavior
   */
  onCloseDialog: () => void
}

function createStorybookAPI(options: MockPsstDialogAPIOptions) {
  const {
    settingsCardData = {},
    requestDelay = 1000,
    errorUids = [],
    onReportFailedContent,
    onCloseDialog,
  } = options

  const finalSettingsData: Mojom.SettingCardData = {
    siteName: settingsCardData.siteName || 'example.com',
    items: settingsCardData.items || [],
  }

  const mockConsentHelper: Mojom.PsstConsentHelperInterface = {
    async performPrivacyTuning(performForUids: string[]) {},

    async closeDialog() {
      onCloseDialog()
    },

    async reportFailedContent() {
      onReportFailedContent()
    },
  }

  const { api, dialogHandler } = createPsstDialogApi(mockConsentHelper)

  mockConsentHelper.performPrivacyTuning = async (performForUids: string[]) => {
    for (const item of finalSettingsData.items) {
      // Simulate request status update
      setTimeout(() => {
        const hasError = errorUids.includes(item.uid)
        dialogHandler.onSetRequestStatus(
          item.uid,
          hasError ? 'Failed to update setting' : null,
        )
      }, Math.random() * requestDelay)
    }
  }

  return { api, dialogHandler, siteData: finalSettingsData }
}
