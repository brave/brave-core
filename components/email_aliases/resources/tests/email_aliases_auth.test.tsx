// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render, screen } from '@testing-library/react'
import { EmailAliasesManagePage } from '../email_aliases'
import { EmailAliasesPanel } from '../email_aliases_panel'
import { useBraveAccountState } from '../content/use_email_aliases'
import type { AccountState } from 'gen/brave/components/brave_account/mojom/brave_account.mojom.m'
import {
  EmailAliasesServiceInterface,
  EmailAliasesPanelHandlerInterface,
  EmailAliasesServiceObserverInterface,
} from 'gen/brave/components/email_aliases/email_aliases.mojom.m'

jest.mock('../content/use_email_aliases', () => ({
  ...jest.requireActual('../content/use_email_aliases'),
  useBraveAccountState: jest.fn(),
}))

const mockUseBraveAccountState = jest.mocked(useBraveAccountState)

const makeLoggedOutAccountState = (): AccountState =>
  ({ loggedOut: {} }) as AccountState

const mockEmailAliasesService: EmailAliasesServiceInterface = {
  updateAlias: jest.fn(),
  deleteAlias: jest.fn(),
  generateAlias: jest.fn(),
  addObserver: jest.fn(),
}

const mockEmailAliasesPanelHandler: EmailAliasesPanelHandlerInterface = {
  onAliasCreated: jest.fn(),
  onManageAliases: jest.fn(),
  onCancelAliasCreation: jest.fn(),
}

const createBindObserver =
  (emailAliasesService: EmailAliasesServiceInterface) =>
  (observer: EmailAliasesServiceObserverInterface) => {
    emailAliasesService.addObserver(observer)
    return () => {}
  }

describe('logged out auth gate', () => {
  beforeEach(() => {
    jest.clearAllMocks()
  })

  describe('EmailAliasesManagePage', () => {
    it.each([
      ['account state is unknown', undefined],
      ['the user is logged out', makeLoggedOutAccountState()],
    ])('renders nothing when %s', (_label, accountState) => {
      mockUseBraveAccountState.mockReturnValue(accountState)

      const { container } = render(
        <EmailAliasesManagePage
          emailAliasesService={mockEmailAliasesService}
          bindObserver={createBindObserver(mockEmailAliasesService)}
        />,
      )

      expect(container).toBeEmptyDOMElement()
      expect(
        screen.queryByText(S.SETTINGS_EMAIL_ALIASES_LIST_TITLE),
      ).not.toBeInTheDocument()
      expect(mockEmailAliasesService.addObserver).not.toHaveBeenCalled()
    })
  })

  describe('EmailAliasesPanel', () => {
    it.each([
      ['account state is unknown', undefined],
      ['the user is logged out', makeLoggedOutAccountState()],
    ])('renders nothing when %s', (_label, accountState) => {
      mockUseBraveAccountState.mockReturnValue(accountState)

      const { container } = render(
        <EmailAliasesPanel
          emailAliasesService={mockEmailAliasesService}
          emailAliasesPanelHandler={mockEmailAliasesPanelHandler}
          bindObserver={createBindObserver(mockEmailAliasesService)}
        />,
      )

      expect(container).toBeEmptyDOMElement()
      expect(
        screen.queryByText(S.SETTINGS_EMAIL_ALIASES_BUBBLE_DESCRIPTION),
      ).not.toBeInTheDocument()
      expect(mockEmailAliasesService.addObserver).not.toHaveBeenCalled()
      expect(mockEmailAliasesService.generateAlias).not.toHaveBeenCalled()
    })
  })
})
