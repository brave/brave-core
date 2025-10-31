// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { act, fireEvent, render, screen, waitFor } from '@testing-library/react'
import { AliasList, ListIntroduction } from '../content/email_aliases_list'
import {
  Alias,
  EmailAliasesServiceInterface,
  MAX_ALIASES,
} from 'gen/brave/components/email_aliases/email_aliases.mojom.m'
import { clickLeoButton } from './test_utils'

// Mock the clipboard API
Object.assign(navigator, {
  clipboard: {
    writeText: jest.fn(),
  },
})

// Mock the email aliases service
const mockEmailAliasesService: EmailAliasesServiceInterface = {
  updateAlias: jest.fn(),
  deleteAlias: jest.fn(),
  generateAlias: jest.fn(),
  requestAuthentication: jest.fn(),
  cancelAuthenticationOrLogout: jest.fn(),
  addObserver: jest.fn(),
}

describe('AliasList', () => {
  const mockAuthEmail = 'test@brave.com'

  const mockAliases: Alias[] = [
    {
      email: 'test1@brave.com',
      note: 'Test Alias 1',
      domains: ['brave.com'],
    },
    {
      email: 'test2@brave.com',
      note: 'Test Alias 2',
      domains: ['brave.com'],
    },
  ]

  const renderAliasList = () => {
    render(
      <AliasList
        aliases={mockAliases}
        authEmail={mockAuthEmail}
        emailAliasesService={mockEmailAliasesService}
      />,
    )
  }

  const waitForTexts = async (expectedTexts: (string | RegExp)[]) => {
    await waitFor(() => {
      expectedTexts.forEach((text) => {
        expect(screen.getByText(text)).toBeInTheDocument()
      })
    })
  }

  const clickMenuItem = async (menuText: string) => {
    const menuItemText = screen.queryAllByText(menuText)[0]
    expect(menuItemText).toBeInTheDocument()

    const menuItem = menuItemText?.closest('leo-menu-item')
    expect(menuItem).toBeInTheDocument()

    if (menuItem) {
      await act(async () => {
        fireEvent.click(menuItem)
      })
    }
  }

  beforeEach(() => {
    jest.clearAllMocks()
    // Mock dialog.showModal
    HTMLDialogElement.prototype.showModal = jest.fn()
    HTMLDialogElement.prototype.close = jest.fn()
  })

  it('renders the alias list', async () => {
    renderAliasList()
    await waitForTexts([
      /test1@brave\.com/,
      /Test Alias 1/,
      /test2@brave\.com/,
      /Test Alias 2/,
    ])
  })

  it('disables create button when max aliases reached', () => {
    const mockOnCreateClicked = jest.fn()
    render(
      <ListIntroduction
        aliasesCount={MAX_ALIASES}
        onCreateClicked={mockOnCreateClicked}
      />,
    )

    // Check if create button is disabled
    const createButton = screen.getByText(
      S.SETTINGS_EMAIL_ALIASES_CREATE_ALIAS_LABEL,
    )
    expect(createButton).toHaveAttribute('isdisabled', 'true')
  })

  it('fires callback when create button is clicked', async () => {
    const mockOnCreateClicked = jest.fn()

    render(
      <ListIntroduction
        aliasesCount={mockAliases.length}
        onCreateClicked={mockOnCreateClicked}
      />,
    )

    const createButton = screen.getByText(
      S.SETTINGS_EMAIL_ALIASES_CREATE_ALIAS_LABEL,
    )
    clickLeoButton(createButton)

    expect(mockOnCreateClicked).toHaveBeenCalled()
  })

  it('shows Edit Alias Modal when edit button is clicked', async () => {
    renderAliasList()

    await clickMenuItem(S.SETTINGS_EMAIL_ALIASES_EDIT)

    await waitForTexts([
      S.SETTINGS_EMAIL_ALIASES_EDIT_ALIAS_TITLE,
      S.SETTINGS_EMAIL_ALIASES_CANCEL_BUTTON,
      S.SETTINGS_EMAIL_ALIASES_SAVE_ALIAS_BUTTON,
    ])
    expect(
      screen.getByPlaceholderText(
        S.SETTINGS_EMAIL_ALIASES_EDIT_NOTE_PLACEHOLDER,
      ),
    ).toBeInTheDocument()
  })

  it('shows Delete Alias Modal when delete button is clicked', async () => {
    renderAliasList()

    await clickMenuItem(S.SETTINGS_EMAIL_ALIASES_DELETE)

    await waitForTexts([
      S.SETTINGS_EMAIL_ALIASES_DELETE_ALIAS_TITLE,
      S.SETTINGS_EMAIL_ALIASES_CANCEL_BUTTON,
      S.SETTINGS_EMAIL_ALIASES_DELETE_ALIAS_BUTTON,
    ])
  })
})
