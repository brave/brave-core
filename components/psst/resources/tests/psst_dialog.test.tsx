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
} from '../ui/api/mock_psst_dialog'
import { clickLeoButton } from './test_utils'

jest.mock('$web-common/locale', () => ({
  getLocale: (key: string) => key,
}))

function setupTest() {
  const consentHelper = createMockConsentHelper({
    closeDialog: jest.fn(),
    applyChanges: jest.fn(),
  })
  const callbackRouter = createMockCallbackRouter({})
  const { api } = createPsstDialogApi(consentHelper, callbackRouter)

  render(
    <PsstDialogAPIProvider {...{ api }}>
      <PsstProgressModal />
    </PsstDialogAPIProvider>,
  )

  return {
    consentHelper,
    callbackRouter,
  }
}

describe('PsstProgressModal with API', () => {
  beforeEach(() => {
    jest.clearAllMocks()
  })

  it('shows title and options area', () => {
    setupTest()

    expect(screen.getByText(S.PSST_CONSENT_DIALOG_TITLE)).toBeInTheDocument()
    expect(
      screen.getByText(S.PSST_CONSENT_DIALOG_OPTIONS_TITLE),
    ).toBeInTheDocument()
  })

  it('displays settings card data when callback router dispatches', async () => {
    const { callbackRouter } = setupTest()

    await act(() => {
      callbackRouter.setSettingsCardData.dispatch(SAMPLE_SETTING_CARD_DATA)
    })

    await waitFor(() => {
      expect(
        screen.getByText(SAMPLE_SETTING_CARD_DATA.siteName),
      ).toBeInTheDocument()
      for (const item of SAMPLE_SETTING_CARD_DATA.items) {
        expect(screen.getByText(item.description)).toBeInTheDocument()
      }
    })
  })

  it('calls closeDialog when close button is clicked', async () => {
    const { consentHelper, callbackRouter } = setupTest()

    await act(() => {
      callbackRouter.setSettingsCardData.dispatch(SAMPLE_SETTING_CARD_DATA)
    })

    await waitFor(() => {
      expect(
        screen.getByText(SAMPLE_SETTING_CARD_DATA.siteName),
      ).toBeInTheDocument()
    })

    const closeButton = document.querySelector('leo-button[fab]')
    expect(closeButton).toBeInTheDocument()
    if (closeButton) {
      clickLeoButton(closeButton)
    }

    await waitFor(() => {
      expect(consentHelper.closeDialog).toHaveBeenCalled()
    })
  })

  it('calls applyChanges when OK button is clicked', async () => {
    const { consentHelper, callbackRouter } = setupTest()

    await act(() => {
      callbackRouter.setSettingsCardData.dispatch(SAMPLE_SETTING_CARD_DATA)
    })

    await waitFor(() => {
      expect(
        screen.getByText(SAMPLE_SETTING_CARD_DATA.siteName),
      ).toBeInTheDocument()
    })

    const okButton = document.getElementById('psst-dialog-ok-btn')
    expect(okButton).toBeInTheDocument()
    if (okButton) {
      clickLeoButton(okButton)
    }

    await waitFor(() => {
      expect(consentHelper.applyChanges).toHaveBeenCalledWith(
        SAMPLE_SETTING_CARD_DATA.siteName,
        expect.any(Array),
      )
    })
  })

  it('calls closeDialog when Cancel button is clicked', async () => {
    const { consentHelper, callbackRouter } = setupTest()

    await act(() => {
      callbackRouter.setSettingsCardData.dispatch(SAMPLE_SETTING_CARD_DATA)
    })

    await waitFor(() => {
      expect(
        screen.getByText(S.PSST_COMPLETE_CONSENT_DIALOG_CANCEL),
      ).toBeInTheDocument()
    })

    const cancelButton = screen
      .getByText(S.PSST_COMPLETE_CONSENT_DIALOG_CANCEL)
      .closest('leo-button')
    if (cancelButton) {
      clickLeoButton(cancelButton)
    }

    await waitFor(() => {
      expect(consentHelper.closeDialog).toHaveBeenCalled()
    })
  })
})
