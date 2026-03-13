// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render, screen, waitFor, act } from '@testing-library/react'
import { createPsstDialogApi } from '../ui/api/psst_dialog_api'
import { PsstDialogAPIProvider } from '../ui/api/psst_dialog_api_context'
import { PsstProgressModal } from '../ui/components/PsstProgressModal'
import {
  createMockConsentHelper,
  createMockCallbackRouter,
  SAMPLE_SETTING_CARD_DATA,
  type MockCallbackRouterWithDispatch,
} from '../ui/api/mock_psst_dialog'

function renderModalWithMocks() {
  const closeDialog = jest.fn()
  const applyChanges = jest.fn()
  const consentHelper = createMockConsentHelper({ closeDialog, applyChanges })
  const callbackRouter = createMockCallbackRouter()
  const { api } = createPsstDialogApi(
    consentHelper as Parameters<typeof createPsstDialogApi>[0],
    callbackRouter as Parameters<typeof createPsstDialogApi>[1],
  )

  render(
    <PsstDialogAPIProvider api={api}>
      <PsstProgressModal />
    </PsstDialogAPIProvider>,
  )

  return {
    closeDialog,
    applyChanges,
    callbackRouter: callbackRouter as unknown as MockCallbackRouterWithDispatch,
  }
}

describe('PsstProgressModal', () => {
  beforeEach(() => {
    jest.clearAllMocks()
  })

  it('renders with empty state before settings data', () => {
    renderModalWithMocks()

    expect(screen.getByText(S.PSST_CONSENT_DIALOG_TITLE)).toBeInTheDocument()
    expect(
      screen.getByText(S.PSST_CONSENT_DIALOG_OPTIONS_TITLE),
    ).toBeInTheDocument()
    expect(
      screen.getByText(S.PSST_COMPLETE_CONSENT_DIALOG_OK),
    ).toBeInTheDocument()
  })

  it('updates options when requestStatus is dispatched', async () => {
    const { callbackRouter } = renderModalWithMocks()

    await act(() => {
      callbackRouter.setSettingsCardData.dispatch(SAMPLE_SETTING_CARD_DATA)
    })

    await waitFor(() => {
      expect(screen.getByText('Allow analytics')).toBeInTheDocument()
      expect(screen.getByText('Allow marketing')).toBeInTheDocument()
      expect(screen.getByText('Essential only')).toBeInTheDocument()
    })

    await act(() => {
      callbackRouter.onSetRequestDone.dispatch(
        'https://example.com/analytics',
        undefined,
      )
    })

    await waitFor(() => {
      expect(screen.getByText('Allow analytics')).toBeInTheDocument()
    })
  })

  it('shows completed state when onSetCompleted is dispatched', async () => {
    const { callbackRouter } = renderModalWithMocks()

    await act(() => {
      callbackRouter.setSettingsCardData.dispatch(SAMPLE_SETTING_CARD_DATA)
    })

    await waitFor(() => {
      expect(
        screen.getByText(SAMPLE_SETTING_CARD_DATA.siteName),
      ).toBeInTheDocument()
    })

    await act(() => {
      callbackRouter.onSetCompleted.dispatch(['check1'], undefined)
    })

    await waitFor(() => {
      expect(
        screen.getByText('PSST_COMPLETE_CONSENT_DIALOG_CLOSE'),
      ).toBeInTheDocument()
      expect(
        screen.getByText('PSST_COMPLETE_CONSENT_DIALOG_REPORT_FAILED'),
      ).toBeInTheDocument()
    })
  })
})
