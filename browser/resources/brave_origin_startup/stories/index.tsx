/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { Meta, StoryObj } from '@storybook/react'
import { App, BraveOriginHandler } from '../components/app'
import '../components/app.css'

const mockHandler: BraveOriginHandler = {
  checkPurchaseState: async () => ({ isPurchased: false }),
  verifyPurchaseId: async (purchaseId: string) => {
    await new Promise((r) => setTimeout(r, 1500))
    if (purchaseId === 'valid') {
      console.log('[Storybook] closeDialog called — purchase verified')
      return { success: true, errorMessage: '' }
    }
    return {
      success: false,
      errorMessage: 'Invalid purchase ID. Please try again.',
    }
  },
  openBuyWindow: () => console.log('[Storybook] openBuyWindow called'),
  closeDialog: () => console.log('[Storybook] closeDialog called'),
  proceedFree: () => console.log('[Storybook] proceedFree called'),
}

function BraveOriginStartupStory() {
  return <App handler={mockHandler} />
}

function BraveOriginStartupLinuxStory() {
  return (
    <App
      handler={mockHandler}
      isLinuxFreeEligible={true}
    />
  )
}

export default {
  title: 'Brave Origin/Startup Dialog',
  component: BraveOriginStartupStory,
  decorators: [
    (Story: any) => (
      <div style={{ width: '500px', height: '500px', margin: '0 auto' }}>
        <Story />
      </div>
    ),
  ],
} satisfies Meta<typeof BraveOriginStartupStory>

export const MainView: StoryObj<typeof BraveOriginStartupStory> = {}

export const LinuxView: StoryObj<typeof BraveOriginStartupLinuxStory> = {
  render: () => <BraveOriginStartupLinuxStory />,
}
