/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { Meta, StoryObj } from '@storybook/react'

import Flex from '$web-common/Flex'
import { PsstReportModal } from './PsstReportModal'
import { OptionStatus, SettingState } from './PsstProgressModal'

const defaultOptionsStatuses: OptionStatus[] = [
  'Disable personalized ads.',
  "Disable sharing additional information with X's business partners.",
  'Another step that failed',
].map((description, index) => ({
  uid: String(index),
  description,
  error: null,
  checked: true,
  disabled: false,
  settingState: SettingState.Failed,
}))

function PsstReportModalStory({
  siteName = 'x.com',
  optionsStatuses = defaultOptionsStatuses,
  isSending = false,
  isSent = false,
}: {
  readonly siteName?: string
  readonly optionsStatuses?: OptionStatus[]
  readonly isSending?: boolean
  readonly isSent?: boolean
}) {
  return (
    <Flex
      direction='row'
      justify='center'
      align='flex-start'
    >
      <PsstReportModal
        siteName={siteName}
        optionsStatuses={optionsStatuses}
        isSending={isSending}
        isSent={isSent}
        onBack={() => console.log('[Storybook] Back')}
        onClose={() => console.log('[Storybook] Close')}
        onSendReport={() => console.log('[Storybook] Send report')}
      />
    </Flex>
  )
}

export default {
  title: 'PSST/Report Failed Steps',
  component: PsstReportModalStory,
  parameters: {
    layout: 'fullscreen',
  },
} satisfies Meta<typeof PsstReportModalStory>

type Story = StoryObj<typeof PsstReportModalStory>

export const Default: Story = {}

export const Sending: Story = {
  args: { isSending: true },
}

export const Sent: Story = {
  args: { isSent: true },
}
