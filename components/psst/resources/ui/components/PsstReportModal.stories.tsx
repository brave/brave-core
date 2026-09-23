/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { Meta, StoryObj } from '@storybook/react'

import * as Mojom from 'gen/brave/components/psst/core/common/psst_ui_common.mojom.m.js'
import Flex from '$web-common/Flex'
import { PsstDialogAPIProvider } from '../api/psst_dialog_api_context'
import { createPsstDialogApi } from '../api/psst_dialog_api'
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

// Mock API where reportFailedContent() resolves after `reportDelay`, so the
// "Send report" button's sending/sent states can be seen by interacting with
// the story directly.
//
// `reportFailedContent` must be defined here rather than reassigned after
// createPsstDialogApi() runs - it's exposed via actionsFor(), which binds
// whichever function is on this object at that point, so a later
// reassignment would never actually be called.
function createStorybookAPI(reportDelay: number) {
  let dialogHandler: Mojom.PsstConsentDialogInterface

  const mockConsentHelper: Mojom.PsstConsentHelperInterface = {
    async performPrivacyTuning() {},
    async closeDialog() {
      console.log('[Storybook] Close')
    },
    async reportFailedContent() {
      console.log('[Storybook] Send report')
      setTimeout(() => dialogHandler.onPsstErrorsReportSent(), reportDelay)
    },
  }

  const result = createPsstDialogApi(mockConsentHelper)
  dialogHandler = result.dialogHandler

  return result.api
}

function PsstReportModalStory({
  siteName = 'x.com',
  optionsStatuses = defaultOptionsStatuses,
  reportDelay = 1500,
}: {
  readonly siteName?: string
  readonly optionsStatuses?: OptionStatus[]
  readonly reportDelay?: number
}) {
  const api = React.useMemo(
    () => createStorybookAPI(reportDelay),
    [reportDelay],
  )

  return (
    <Flex
      direction='row'
      justify='center'
      align='flex-start'
    >
      <PsstDialogAPIProvider
        api={api}
        siteData={undefined}
      >
        <PsstReportModal
          siteName={siteName}
          optionsStatuses={optionsStatuses}
          onBack={() => console.log('[Storybook] Back')}
        />
      </PsstDialogAPIProvider>
    </Flex>
  )
}

export default {
  title: 'PSST/Report Failed Steps',
  component: PsstReportModalStory,
  parameters: {
    layout: 'fullscreen',
  },
  argTypes: {
    reportDelay: {
      control: { type: 'range', min: 0, max: 5000, step: 100 },
      description: 'Simulated delay before the report is marked as sent',
    },
  },
} satisfies Meta<typeof PsstReportModalStory>

type Story = StoryObj<typeof PsstReportModalStory>

// Sending/sent are internal state of PsstReportModal, driven by the report
// being sent through the (mocked) API - so we simulate a real click on the
// button to reach those states, rather than trying to force them via props.
function clickSendReportButton(canvasElement: HTMLElement) {
  const button = Array.from(canvasElement.querySelectorAll('leo-button')).find(
    (el) => el.textContent?.trim() === 'Send report',
  )
  ;(button as HTMLElement | undefined)?.click()
}

/**
 * Click "Send report" to see the sending/sent states.
 */
export const Default: Story = {}

export const Sending: Story = {
  args: {
    // Long enough that the report never resolves during this story.
    reportDelay: 60_000,
  },
  play: async ({ canvasElement }) => {
    clickSendReportButton(canvasElement)
  },
}

export const Sent: Story = {
  args: {
    reportDelay: 0,
  },
  play: async ({ canvasElement }) => {
    clickSendReportButton(canvasElement)
    await new Promise((resolve) => setTimeout(resolve, 100))
  },
}
