/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { Meta, StoryObj } from '@storybook/react'

import { MockScenario, mockScenarios } from '../api/mock_scenarios'
import { createTransferApiMock } from '../api/transfer_api_mock'
import { TransferApiProvider } from '../api/transfer_api_context'
import { App, Tab } from './app'

interface Props {
  scenario: MockScenario
  tab: Tab
  // Simulates the iOS host (isIOS and isMobile load-time flags).
  isIOS: boolean
}

function StorybookApp(props: Props) {
  const api = React.useMemo(() => {
    const options = mockScenarios[props.scenario]()
    if (props.isIOS) {
      options.isIOS = true
      options.isMobile = true
    }
    return createTransferApiMock(options)
  }, [props.scenario, props.isIOS])

  return (
    <TransferApiProvider
      key={`${props.scenario}-${props.tab}-${props.isIOS}`}
      {...api}
    >
      <App initialTab={props.tab} />
    </TransferApiProvider>
  )
}

export default {
  title: 'Brave Transfer/App',
  component: StorybookApp,
  parameters: { layout: 'fullscreen' },
  argTypes: {
    scenario: {
      control: 'select',
      options: Object.keys(mockScenarios),
    },
    tab: {
      control: 'inline-radio',
      options: ['devices', 'inbox', 'settings'],
    },
  },
  args: {
    scenario: 'devices',
    tab: 'devices',
    isIOS: false,
  },
} satisfies Meta<typeof StorybookApp>

type Story = StoryObj<typeof StorybookApp>

export const Empty: Story = { args: { scenario: 'empty' } }

export const DeviceList: Story = { args: { scenario: 'devices' } }

export const IncomingOffer: Story = { args: { scenario: 'incomingOffer' } }

export const Inbox: Story = { args: { scenario: 'inbox', tab: 'inbox' } }

export const EmptyInbox: Story = {
  args: { scenario: 'emptyInbox', tab: 'inbox' },
}

export const Settings: Story = {
  args: { scenario: 'settings', tab: 'settings' },
}

export const SyncOff: Story = { args: { scenario: 'syncOff' } }

export const IOSPermissions: Story = {
  args: { scenario: 'iosPermissions', isIOS: true },
}

export const IOSInbox: Story = {
  args: { scenario: 'inbox', tab: 'inbox', isIOS: true },
}

export const IOSSettings: Story = {
  args: { scenario: 'settings', tab: 'settings', isIOS: true },
}
