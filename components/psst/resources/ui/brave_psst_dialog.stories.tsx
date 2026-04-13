/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { Meta, StoryObj } from '@storybook/react'
import { setIconBasePath } from '@brave/leo/react/icon'

import { createMockPsstDialogAPI } from './api/psst_dialog_api_mock'
import PsstDlgContainer from './containers/App'
import * as Mojom from 'gen/brave/components/psst/common/psst_ui_common.mojom.m.js'
import Flex from '$web-common/Flex'

// Set icon path for Storybook
setIconBasePath('/icons')

function PsstDialogStory({
  autoLoadSettings = true,
  requestDelay = 1000,
  errorUrls = [] as string[],
  siteName = 'example.com',
  items = [] as Mojom.SettingCardDataItem[],
}: {
  readonly autoLoadSettings?: boolean
  readonly requestDelay?: number
  readonly errorUrls?: string[]
  readonly siteName?: string
  readonly items?: Mojom.SettingCardDataItem[]
}) {
  const mockAPI = React.useMemo(() => {
    return createMockPsstDialogAPI({
      autoLoadSettings,
      requestDelay,
      errorUrls,
      settingsCardData: {
        siteName: siteName,
        items,
      },
      onCloseDialog: () => console.log('[Storybook] Dialog closed'),
    })
  }, [autoLoadSettings, requestDelay, errorUrls, siteName, items])

  return (
    <Flex
      direction='row'
      justify='center'
      align='flex-start'
    >
      <PsstDlgContainer apiContext={mockAPI} />
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
    autoLoadSettings: {
      control: 'boolean',
      description: 'Whether to automatically load settings data',
    },
    requestDelay: {
      control: { type: 'range', min: 100, max: 5000, step: 100 },
      description: 'Simulated network delay in milliseconds',
    },
    errorUrls: {
      control: 'object',
      description: 'Array of URLs that should simulate errors',
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
    autoLoadSettings: true,
    requestDelay: 1000,
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
    errorUrls: [],
  },
}

/**
 * Dialog with some failing requests to show error handling
 */
export const WithErrors: Story = {
  args: {
    siteName: 'example.com',
    autoLoadSettings: true,
    requestDelay: 1500,
    items: [
      {
        url: 'https://example.com/cookies',
        description: 'Manage cookie preferences and tracking protection',
      },
      {
        url: 'https://problematic-site.com/analytics',
        description: 'Control analytics and data collection settings',
      },
      {
        url: 'https://example.com/ads',
        description: 'Configure advertising preferences',
      },
      {
        url: 'https://problematic-site.com/location',
        description: 'Manage location data sharing',
      },
    ],
    errorUrls: [
      'https://problematic-site.com/analytics',
      'https://problematic-site.com/location',
    ],
  },
}
