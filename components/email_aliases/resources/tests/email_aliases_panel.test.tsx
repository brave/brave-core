// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render, screen, waitFor } from '@testing-library/react'
import { EmailAliasesPanelConnected } from '../email_aliases_panel'
import { clickLeoButton } from './test_utils'
import {
  EmailAliasesServiceInterface,
  EmailAliasesPanelHandlerInterface,
  EmailAliasesServiceObserverInterface,
} from 'gen/brave/components/email_aliases/email_aliases.mojom.m'
import type { AccountState } from 'gen/brave/components/brave_account/mojom/brave_account.mojom.m'

const panelAccountLoggedIn = {
  loggedIn: { email: 'test@brave.com' },
} as AccountState

const mockEmailAliasesPanelHandler: EmailAliasesPanelHandlerInterface = {
  onAliasCreated: jest.fn(),
  onManageAliases: jest.fn(),
  onCancelAliasCreation: jest.fn(),
}

const mockEmailAliasesService: EmailAliasesServiceInterface = {
  updateAlias: jest.fn(),
  deleteAlias: jest.fn(),
  generateAlias: jest.fn(),
  requestAuthentication: jest.fn(),
  cancelAuthenticationOrLogout: jest.fn(),
  addObserver: jest.fn(),
}

const createBindObserver =
  (emailAliasesService: EmailAliasesServiceInterface) =>
  (observer: EmailAliasesServiceObserverInterface) => {
    return () => {}
  }

describe('PanelConnected', () => {
  beforeEach(() => {
    jest.clearAllMocks()
    mockEmailAliasesService.generateAlias = jest
      .fn()
      .mockResolvedValue('generated@brave.com')
  })

  it('manage button', async () => {
    mockEmailAliasesPanelHandler.onManageAliases = jest.fn()

    render(
      <EmailAliasesPanelConnected
        emailAliasesService={mockEmailAliasesService}
        emailAliasesPanelHandler={mockEmailAliasesPanelHandler}
        bindObserver={createBindObserver(mockEmailAliasesService)}
        accountStateOverride={panelAccountLoggedIn}
      />,
    )

    const button = screen.getByText(S.SETTINGS_EMAIL_ALIASES_MANAGE_BUTTON)
    clickLeoButton(button)

    await waitFor(() => {
      expect(mockEmailAliasesPanelHandler.onManageAliases).toHaveBeenCalled()
    })
  })

  it('cancel button', async () => {
    mockEmailAliasesPanelHandler.onCancelAliasCreation = jest.fn()

    render(
      <EmailAliasesPanelConnected
        emailAliasesService={mockEmailAliasesService}
        emailAliasesPanelHandler={mockEmailAliasesPanelHandler}
        bindObserver={createBindObserver(mockEmailAliasesService)}
        accountStateOverride={panelAccountLoggedIn}
      />,
    )

    const button = screen.getByText(S.SETTINGS_EMAIL_ALIASES_CANCEL_BUTTON)
    clickLeoButton(button)

    await waitFor(() => {
      expect(
        mockEmailAliasesPanelHandler.onCancelAliasCreation,
      ).toHaveBeenCalled()
    })
  })

  it('create button', async () => {
    mockEmailAliasesPanelHandler.onAliasCreated = jest.fn()

    render(
      <EmailAliasesPanelConnected
        emailAliasesService={mockEmailAliasesService}
        emailAliasesPanelHandler={mockEmailAliasesPanelHandler}
        bindObserver={createBindObserver(mockEmailAliasesService)}
        accountStateOverride={panelAccountLoggedIn}
      />,
    )

    // Wait for initial generation
    await waitFor(() => {
      expect(mockEmailAliasesService.generateAlias).toHaveBeenCalled()
      expect(screen.queryByTestId('loading-icon')).not.toBeInTheDocument()
      const generatedEmailContainer = screen.getByTestId('generated-email')
      expect(generatedEmailContainer).toHaveTextContent('generated@brave.com')
    })

    const button = screen.getByText(
      S.SETTINGS_EMAIL_ALIASES_CREATE_ALIAS_BUTTON,
    )
    expect(button).toHaveAttribute('isdisabled', 'false')
    clickLeoButton(button)

    await waitFor(() => {
      expect(mockEmailAliasesPanelHandler.onAliasCreated).toHaveBeenCalledWith(
        'generated@brave.com',
      )
    })
  })
})
