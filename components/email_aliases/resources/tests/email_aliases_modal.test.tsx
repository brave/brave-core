// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render, screen, waitFor } from '@testing-library/react'
import {
  EmailAliasModal,
  DeleteAliasModal,
  EmailAliasModalActionType
} from '../content/email_aliases_modal'

import { clickLeoButton } from './test_utils'
import {
  Alias,
  EmailAliasesServiceInterface,
  EmailAliasesPanelHandler
} from 'gen/brave/components/email_aliases/email_aliases.mojom.m'

const emptyResultPromise = Promise.resolve({})

// Mock the email aliases service
const mockEmailAliasesService: EmailAliasesServiceInterface = {
  updateAlias: jest.fn(),
  deleteAlias: jest.fn(),
  generateAlias: jest.fn(),
  requestAuthentication: jest.fn(),
  cancelAuthenticationOrLogout: jest.fn(),
  addObserver: jest.fn(),
}

const mockEmailAliasesHandler: EmailAliasesPanelHandler {
  onComplete: jest.fn(),
  onManage: jest.fn(),
  onCancel: jest.fn()
}

describe('EmailAliasModal', () => {
  const mockOnReturnToMain = jest.fn()
  const mockEmail = 'test@brave.com'
  const mockAlias: Alias = {
    email: 'existing@brave.com',
    note: 'Existing Alias',
    domains: ['brave.com'],
  }

  beforeEach(() => {
    jest.clearAllMocks()
    mockEmailAliasesService.updateAlias = jest
      .fn()
      .mockResolvedValue(emptyResultPromise)
    mockEmailAliasesService.deleteAlias = jest
      .fn()
      .mockImplementation(async () => {
        await new Promise((resolve) => setTimeout(resolve, 0))
        return emptyResultPromise
      })
    mockEmailAliasesService.generateAlias = jest
      .fn()
      .mockResolvedValue('generated@brave.com')
  })

  it('renders create mode correctly', async () => {
    render(
      <EmailAliasModal
        editing={false}
        mainEmail={mockEmail}
        aliasCount={0}
        onReturnToMain={mockOnReturnToMain}
        emailAliasesService={mockEmailAliasesService}
      />,
    )

    // Check title and description
    expect(
      screen.getByText(S.SETTINGS_EMAIL_ALIASES_CREATE_ALIAS_TITLE),
    ).toBeInTheDocument()

    // Check that generate alias was called
    await waitFor(() => {
      expect(mockEmailAliasesService.generateAlias).toHaveBeenCalled()
    })
  })

  it('renders edit mode correctly', () => {
    const mockEditAlias: Alias = {
      email: 'existing@brave.com',
      note: 'Existing Alias',
      domains: ['brave.com'],
    }

    render(
      <EmailAliasModal
        editing={true}
        editAlias={mockEditAlias}
        mainEmail={mockEmail}
        aliasCount={0}
        onReturnToMain={mockOnReturnToMain}
        emailAliasesService={mockEmailAliasesService}
      />,
    )

    // Check title
    expect(
      screen.getByText(S.SETTINGS_EMAIL_ALIASES_EDIT_ALIAS_TITLE),
    ).toBeInTheDocument()

    // Check that existing alias is displayed
    expect(screen.getByText(/existing@brave\.com/)).toBeInTheDocument()
    expect(
      screen.getByPlaceholderText(
        S.SETTINGS_EMAIL_ALIASES_EDIT_NOTE_PLACEHOLDER,
      ),
    ).toHaveValue('Existing Alias')
  })

  it(
    'renders delete mode correctly and calls deleteAlias when delete button '
      + 'is clicked',
    async () => {
      const mockAlias: Alias = {
        email: 'existing@brave.com',
        note: 'Existing Alias',
        domains: ['brave.com'],
      }
      render(
        <DeleteAliasModal
          onReturnToMain={mockOnReturnToMain}
          alias={mockAlias}
          emailAliasesService={mockEmailAliasesService}
        />,
      )

      // Check expected strings
      expect(
        screen.getByText(S.SETTINGS_EMAIL_ALIASES_DELETE_ALIAS_TITLE),
      ).toBeInTheDocument()
      expect(
        screen.getByText(S.SETTINGS_EMAIL_ALIASES_DELETE_ALIAS_DESCRIPTION),
      ).toBeInTheDocument()
      expect(
        screen.getByText(S.SETTINGS_EMAIL_ALIASES_DELETE_ALIAS_BUTTON),
      ).toBeInTheDocument()
      expect(
        screen.getByText(S.SETTINGS_EMAIL_ALIASES_DELETE_WARNING),
      ).toBeInTheDocument()

      // Click delete button
      const deleteButton = screen.getByText(
        S.SETTINGS_EMAIL_ALIASES_DELETE_ALIAS_BUTTON,
      )
      clickLeoButton(deleteButton)

      // Check that deleteAlias was called
      await waitFor(() => {
        expect(mockEmailAliasesService.deleteAlias).toHaveBeenCalled()
        expect(deleteButton).toHaveAttribute('isdisabled', 'false')
      })
    },
  )

  it('handles alias creation', async () => {
    render(
      <EmailAliasModal
        editing={false}
        mainEmail={mockEmail}
        aliasCount={0}
        onReturnToMain={mockOnReturnToMain}
        emailAliasesService={mockEmailAliasesService}
      />,
    )

    // Wait for generated alias
    await waitFor(() => {
      expect(mockEmailAliasesService.generateAlias).toHaveBeenCalled()
    })

    // Wait for loading to complete and alias to be generated
    await waitFor(() => {
      expect(
        screen.getByPlaceholderText(
          S.SETTINGS_EMAIL_ALIASES_EDIT_NOTE_PLACEHOLDER,
        ),
      ).toBeInTheDocument()
      expect(screen.queryByTestId('loading-icon')).not.toBeInTheDocument()
      const generatedEmailContainer = screen.getByTestId('generated-email')
      expect(generatedEmailContainer).toHaveTextContent('generated@brave.com')
    })

    // Ensure the save button is enabled
    const saveButton = screen.getByText(
      S.SETTINGS_EMAIL_ALIASES_CREATE_ALIAS_BUTTON,
    )
    expect(saveButton).toHaveAttribute('isdisabled', 'false')

    // Click save button
    clickLeoButton(saveButton)

    // Check that updateAlias was called
    await waitFor(() => {
      expect(mockEmailAliasesService.updateAlias).toHaveBeenCalled()
      expect(mockOnReturnToMain).toHaveBeenCalled()
    })
  })

  it('handles alias regeneration', async () => {
    render(
      <EmailAliasModal
        editing={false}
        mainEmail={mockEmail}
        aliasCount={0}
        onReturnToMain={mockOnReturnToMain}
        emailAliasesService={mockEmailAliasesService}
      />,
    )

    // Wait for initial generation
    await waitFor(() => {
      expect(mockEmailAliasesService.generateAlias).toHaveBeenCalled()
      expect(
        screen.getByTitle(S.SETTINGS_EMAIL_ALIASES_REFRESH_BUTTON_TITLE),
      ).toBeInTheDocument()
    })

    // Click regenerate button
    const regenerateButton = screen.getByTitle(
      S.SETTINGS_EMAIL_ALIASES_REFRESH_BUTTON_TITLE,
    )
    clickLeoButton(regenerateButton)

    // Check that generateAlias was called again
    await waitFor(() => {
      expect(mockEmailAliasesService.generateAlias).toHaveBeenCalledTimes(2)
    })
  })

  it('shows limit reached message in bubble mode', async () => {
    render(
      <EmailAliasModal
        editing={false}
        mainEmail={mockEmail}
        aliasCount={5}
        onReturnToMain={mockOnReturnToMain}
        emailAliasesService={mockEmailAliasesService}
        bubble={true}
      />,
    )

    // Wait for limit check
    await waitFor(() => {
      expect(
        screen.getByText(S.SETTINGS_EMAIL_ALIASES_BUBBLE_LIMIT_REACHED),
      ).toBeInTheDocument()
    })
  })

  it('shows loading state while generating alias', async () => {
    const aliasEmail = 'new@brave.com'
    mockEmailAliasesService.generateAlias = jest
      .fn()
      .mockImplementation(() => Promise.resolve(aliasEmail))

    render(
      <EmailAliasModal
        editing={false}
        mainEmail={mockEmail}
        aliasCount={0}
        onReturnToMain={mockOnReturnToMain}
        emailAliasesService={mockEmailAliasesService}
      />,
    )

    // Initially should show loading state but not the button
    const loadingIcon = document.querySelector('leo-progressring')
    expect(loadingIcon).toBeInTheDocument()
    const regenerateButton = screen.queryByTitle(
      S.SETTINGS_EMAIL_ALIASES_REFRESH_BUTTON_TITLE,
    )
    expect(regenerateButton).not.toBeInTheDocument()

    // Wait for generation to complete. Should now show the button
    await waitFor(() => {
      expect(loadingIcon).not.toBeInTheDocument()
      const regenerateButton = screen.queryByTitle(
        S.SETTINGS_EMAIL_ALIASES_REFRESH_BUTTON_TITLE,
      )
      expect(regenerateButton).toBeInTheDocument()
      const saveButton = screen.getByText(
        S.SETTINGS_EMAIL_ALIASES_CREATE_ALIAS_BUTTON,
      )
      expect(saveButton).toHaveAttribute('isdisabled', 'false')
      const aliasEmailBox = screen.getByTestId('generated-email')
      expect(aliasEmailBox).toHaveTextContent(aliasEmail)
    })
  })

  it('shows error message when generating alias fails', async () => {
    mockEmailAliasesService.generateAlias = jest
      .fn()
      .mockImplementation(() =>
        Promise.reject(S.SETTINGS_EMAIL_ALIASES_GENERATE_ERROR),
      )

    render(
      <EmailAliasModal
        editing={false}
        mainEmail={mockEmail}
        aliasCount={0}
        onReturnToMain={mockOnReturnToMain}
        emailAliasesService={mockEmailAliasesService}
      />,
    )

    // Wait for error message to be displayed. Create button should be disabled.
    await waitFor(() => {
      const loadingIcon = document.querySelector('leo-progressring')
      expect(loadingIcon).not.toBeInTheDocument()
      const regenerateButton = screen.queryByTitle(
        S.SETTINGS_EMAIL_ALIASES_REFRESH_BUTTON_TITLE,
      )
      expect(regenerateButton).toBeInTheDocument()
      expect(
        screen.getByText(S.SETTINGS_EMAIL_ALIASES_GENERATE_ERROR),
      ).toBeInTheDocument()
      const saveButton = screen.getByText(
        S.SETTINGS_EMAIL_ALIASES_CREATE_ALIAS_BUTTON,
      )
      expect(saveButton).toHaveAttribute('isdisabled', 'true')
      const aliasEmailBox = screen.getByTestId('generated-email')
      expect(aliasEmailBox).toHaveTextContent('')
    })
  })

  it('handles alias updates', async () => {
    const mockEditAlias: Alias = {
      email: 'existing@brave.com',
      note: 'Existing Alias',
      domains: ['brave.com'],
    }

    render(
      <EmailAliasModal
        editing={true}
        editAlias={mockEditAlias}
        mainEmail={mockEmail}
        aliasCount={0}
        onReturnToMain={mockOnReturnToMain}
        emailAliasesService={mockEmailAliasesService}
      />,
    )

    // Wait for initial state to be set
    await waitFor(() => {
      expect(screen.getByText(/existing@brave\.com/)).toBeInTheDocument()
      expect(
        screen.getByPlaceholderText(
          S.SETTINGS_EMAIL_ALIASES_EDIT_NOTE_PLACEHOLDER,
        ),
      ).toHaveValue('Existing Alias')
    })

    // Ensure the save button is enabled
    const saveButton = screen.getByText(
      S.SETTINGS_EMAIL_ALIASES_SAVE_ALIAS_BUTTON,
    )
    expect(saveButton).toHaveAttribute('isdisabled', 'false')

    // Click save button
    clickLeoButton(saveButton)

    // Check that updateAlias was called
    await waitFor(() => {
      expect(mockEmailAliasesService.updateAlias).toHaveBeenCalledWith(
        'existing@brave.com',
        'Existing Alias',
      )
      expect(mockOnReturnToMain).toHaveBeenCalled()
    })
  })

  const aliases: Alias[] = [
    {
      email: 'new@brave.com',
      note: 'New Alias',
      domains: undefined,
    },
    {
      email: 'existing@brave.com',
      note: 'Existing Alias',
      domains: ['brave.com'],
    },
  ]

  for (const alias of aliases) {
    it('shows error message when creating or editing alias fails', async () => {
      mockEmailAliasesService.updateAlias = jest
        .fn()
        .mockImplementation(() =>
          Promise.reject(S.SETTINGS_EMAIL_ALIASES_UPDATE_ALIAS_ERROR),
        )

      const isEditing = alias.email === 'existing@brave.com'

      render(
        <EmailAliasModal
          editing={isEditing}
          editAlias={alias}
          mainEmail={mockEmail}
          aliasCount={0}
          onReturnToMain={mockOnReturnToMain}
          emailAliasesService={mockEmailAliasesService}
        />,
      )

      // Wait for error message to appear. Create/Edit button should be enabled.
      const saveButton = isEditing
        ? screen.getByText(S.SETTINGS_EMAIL_ALIASES_SAVE_ALIAS_BUTTON)
        : screen.getByText(S.SETTINGS_EMAIL_ALIASES_CREATE_ALIAS_BUTTON)

      await waitFor(() => {
        expect(saveButton).toHaveAttribute('isdisabled', 'false')
      })

      // Click save button
      clickLeoButton(saveButton)

      // Check that updateAlias was called and error message is displayed.
      await waitFor(() => {
        expect(mockEmailAliasesService.updateAlias).toHaveBeenCalled()
        expect(
          screen.getByText(S.SETTINGS_EMAIL_ALIASES_UPDATE_ALIAS_ERROR),
        ).toBeInTheDocument()
      })
    })
  }

  it('shows error message when deleting alias fails', async () => {
    mockEmailAliasesService.deleteAlias = jest
      .fn()
      .mockImplementation(() =>
        Promise.reject(S.SETTINGS_EMAIL_ALIASES_DELETE_ALIAS_ERROR),
      )

    render(
      <DeleteAliasModal
        onReturnToMain={mockOnReturnToMain}
        alias={mockAlias}
        emailAliasesService={mockEmailAliasesService}
      />,
    )

    // Click delete button
    const deleteButton = screen.getByText(
      S.SETTINGS_EMAIL_ALIASES_DELETE_ALIAS_BUTTON,
    )
    clickLeoButton(deleteButton)

    // Wait for error message to appear. Delete button should be enabled.
    await waitFor(() => {
      expect(mockEmailAliasesService.deleteAlias).toHaveBeenCalled()
      expect(
        screen.getByText(S.SETTINGS_EMAIL_ALIASES_DELETE_ALIAS_ERROR),
      ).toBeInTheDocument()
    })
  })

  it('manage button in panel', async () => {
      render(
        <EmailAliasModal
          editing={false}
          mainEmail={mockEmail}
          aliasCount={0}
          onReturnToMain={mockOnReturnToMain}
          emailAliasesService={mockEmailAliasesService}
          bubble={true}
        />,
      )

    const manageButton = screen.getByText('emailAliasesManageButton')
    clickLeoButton(manageButton)

    await waitFor(() => {
      expect(mockOnReturnToMain).toHaveBeenCalledWith(
        {type:EmailAliasModalActionType.Manage}
      )
    })
  })
})
