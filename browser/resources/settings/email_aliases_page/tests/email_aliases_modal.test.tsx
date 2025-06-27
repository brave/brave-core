// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render, screen, waitFor, act } from '@testing-library/react'
import { EmailAliasModal, DeleteAliasModal }
  from '../content/email_aliases_modal'

import { clickLeoButton } from './test_utils'
import { Alias, EmailAliasesServiceInterface, GenerateAliasResult }
  from "gen/brave/components/email_aliases/email_aliases.mojom.m"

// Mock the email aliases service
const mockEmailAliasesService: EmailAliasesServiceInterface = {
  updateAlias: jest.fn(),
  deleteAlias: jest.fn(),
  generateAlias: jest.fn(),
  requestAuthentication: jest.fn(),
  cancelAuthenticationOrLogout: jest.fn(),
  addObserver: jest.fn(),
  removeObserver: jest.fn()
}

describe('EmailAliasModal', () => {
  const mockOnReturnToMain = jest.fn()
  const mockEmail = 'test@brave.com'
  const mockAlias: Alias = {
    email: 'existing@brave.com',
    note: 'Existing Alias',
    domains: ['brave.com']
  }

  beforeEach(() => {
    jest.clearAllMocks()
    mockEmailAliasesService.updateAlias = jest.fn().mockResolvedValue(
      Promise.resolve({ errorMessage: null }))
    mockEmailAliasesService.deleteAlias = jest.fn().mockImplementation(
      async () => {
        await new Promise(resolve => setTimeout(resolve, 0))
        return { errorMessage: null }
      })
    mockEmailAliasesService.generateAlias = jest.fn()
      .mockResolvedValue({
        result: {
          errorMessage: null, aliasEmail: 'generated@brave.com'
        }
      })
  })

  it('renders create mode correctly', async () => {
    render(
      <EmailAliasModal
        editing={false}
        mainEmail={mockEmail}
        aliasCount={0}
        onReturnToMain={mockOnReturnToMain}
        emailAliasesService={mockEmailAliasesService}
      />
    )

    // Check title and description
    expect(screen.getByText('emailAliasesCreateAliasTitle'))
      .toBeInTheDocument()

    // Check that generate alias was called
    await waitFor(() => {
      expect(mockEmailAliasesService.generateAlias).toHaveBeenCalled()
    })
  })

  it('renders edit mode correctly', () => {
    const mockEditAlias: Alias = {
      email: 'existing@brave.com',
      note: 'Existing Alias',
      domains: ['brave.com']
    }

    render(
      <EmailAliasModal
        editing={true}
        editAlias={mockEditAlias}
        mainEmail={mockEmail}
        aliasCount={0}
        onReturnToMain={mockOnReturnToMain}
        emailAliasesService={mockEmailAliasesService}
      />
    )

    // Check title
    expect(screen.getByText('emailAliasesEditAliasTitle'))
      .toBeInTheDocument()

    // Check that existing alias is displayed
    expect(screen.getByText(/existing@brave\.com/)).toBeInTheDocument()
    expect(screen.getByPlaceholderText('emailAliasesEditNotePlaceholder'))
      .toHaveValue('Existing Alias')
  })

  it('renders delete mode correctly and calls deleteAlias when delete button ' +
    'is clicked', async () => {
    const mockAlias: Alias = {
      email: 'existing@brave.com',
      note: 'Existing Alias',
      domains: ['brave.com']
    }
    render(
      <DeleteAliasModal
        onReturnToMain={mockOnReturnToMain}
        alias={mockAlias}
        emailAliasesService={mockEmailAliasesService}
      />
    )

    // Check expected strings
    expect(screen.getByText('emailAliasesDeleteAliasTitle'))
      .toBeInTheDocument()
    expect(screen.getByText('emailAliasesDeleteAliasDescription'))
      .toBeInTheDocument()
    expect(screen.getByText('emailAliasesDeleteAliasButton'))
      .toBeInTheDocument()
    expect(screen.getByText('emailAliasesDeleteWarning'))
      .toBeInTheDocument()

    // Click delete button
    const deleteButton = screen.getByText('emailAliasesDeleteAliasButton')
    clickLeoButton(deleteButton)

    await waitFor(() => {
      expect(deleteButton).toHaveAttribute('isdisabled', 'true')
    })

    // Check that deleteAlias was called
    await waitFor(() => {
      expect(mockEmailAliasesService.deleteAlias).toHaveBeenCalled()
      expect(deleteButton).toHaveAttribute('isdisabled', 'false')
    })
  })

  it('handles alias creation', async () => {
    render(
      <EmailAliasModal
      editing={false}
      mainEmail={mockEmail}
      aliasCount={0}
      onReturnToMain={mockOnReturnToMain}
      emailAliasesService={mockEmailAliasesService}
    />)

    // Wait for generated alias
    await waitFor(() => {
      expect(mockEmailAliasesService.generateAlias).toHaveBeenCalled()
    })

    // Wait for loading to complete and alias to be generated
    await waitFor(() => {
      expect(screen.getByPlaceholderText('emailAliasesEditNotePlaceholder'))
        .toBeInTheDocument()
      expect(screen.queryByTestId('loading-icon')).not.toBeInTheDocument()
      const generatedEmailContainer = screen.getByTestId('generated-email')
      expect(generatedEmailContainer).toHaveTextContent('generated@brave.com')
    })

    // Ensure the save button is enabled
    const saveButton = screen.getByText('emailAliasesCreateAliasButton')
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
      />
    )

    // Wait for initial generation
    await waitFor(() => {
      expect(mockEmailAliasesService.generateAlias).toHaveBeenCalled()
      expect(screen.getByTitle('emailAliasesRefreshButtonTitle'))
        .toBeInTheDocument()
    })

    // Click regenerate button
    const regenerateButton = screen.getByTitle('emailAliasesRefreshButtonTitle')
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
      />
    )

    // Wait for limit check
    await waitFor(() => {
      expect(screen.getByText('emailAliasesBubbleLimitReached'))
        .toBeInTheDocument()
    })
  })

  it('shows loading state while generating alias', async () => {
    const aliasEmail = 'new@brave.com'
    mockEmailAliasesService.generateAlias = jest.fn().mockImplementation(
      () => Promise.resolve<{ result: GenerateAliasResult }>({
        result: { aliasEmail, errorMessage: undefined } }))

    render(
      <EmailAliasModal
        editing={false}
        mainEmail={mockEmail}
        aliasCount={0}
        onReturnToMain={mockOnReturnToMain}
        emailAliasesService={mockEmailAliasesService}
      />
    )

    // Initially should show loading state but not the button
    const loadingIcon = document.querySelector('leo-progressring')
    expect(loadingIcon).toBeInTheDocument()
    const regenerateButton = screen.queryByTitle(
      'emailAliasesRefreshButtonTitle')
    expect(regenerateButton).not.toBeInTheDocument()

    // Wait for generation to complete. Should now show the button
    await waitFor(() => {
      expect(loadingIcon).not.toBeInTheDocument()
      const regenerateButton = screen.queryByTitle(
        'emailAliasesRefreshButtonTitle')
      expect(regenerateButton).toBeInTheDocument()
      const saveButton = screen.getByText('emailAliasesCreateAliasButton')
      expect(saveButton).toHaveAttribute('isdisabled', 'false')
      const aliasEmailBox = screen.getByTestId('generated-email')
      expect(aliasEmailBox).toHaveTextContent(aliasEmail)
    })
  })

  it("shows error message when generating alias fails", async () => {
    mockEmailAliasesService.generateAlias = jest.fn().mockImplementation(
      () => Promise.resolve<{ result: GenerateAliasResult }>({
        result: {
          errorMessage: 'emailAliasesGenerateError',
          aliasEmail: undefined
        }
      }))

    render(
      <EmailAliasModal
        editing={false}
        mainEmail={mockEmail}
        aliasCount={0}
        onReturnToMain={mockOnReturnToMain}
        emailAliasesService={mockEmailAliasesService}
      />
    )

    // Wait for error message to be displayed. Create button should be disabled.
    await waitFor(() => {
      const loadingIcon = document.querySelector('leo-progressring')
      expect(loadingIcon).not.toBeInTheDocument()
      const regenerateButton = screen.queryByTitle(
        'emailAliasesRefreshButtonTitle')
      expect(regenerateButton).toBeInTheDocument()
      expect(screen.getByText('emailAliasesGenerateError'))
        .toBeInTheDocument()
      const saveButton = screen.getByText('emailAliasesCreateAliasButton')
      expect(saveButton).toHaveAttribute('isdisabled', 'true')
      const aliasEmailBox = screen.getByTestId('generated-email')
      expect(aliasEmailBox).toHaveTextContent('')
    })
  })

  it('handles alias updates', async () => {
    const mockEditAlias: Alias = {
      email: 'existing@brave.com',
      note: 'Existing Alias',
      domains: ['brave.com']
    }

    render(
      <EmailAliasModal
        editing={true}
        editAlias={mockEditAlias}
        mainEmail={mockEmail}
        aliasCount={0}
        onReturnToMain={mockOnReturnToMain}
        emailAliasesService={mockEmailAliasesService}
      />
    )

    // Wait for initial state to be set
    await waitFor(() => {
      expect(screen.getByText(/existing@brave\.com/)).toBeInTheDocument()
      expect(screen.getByPlaceholderText('emailAliasesEditNotePlaceholder'))
        .toHaveValue('Existing Alias')
    })

    // Ensure the save button is enabled
    const saveButton = screen.getByText('emailAliasesSaveAliasButton')
    expect(saveButton).toHaveAttribute('isdisabled', 'false')

    // Click save button
    saveButton.shadowRoot?.querySelector('button')?.click()

    // Check that updateAlias was called
    await waitFor(() => {
      expect(mockEmailAliasesService.updateAlias).toHaveBeenCalledWith(
        'existing@brave.com',
        'Existing Alias'
      )
      expect(mockOnReturnToMain).toHaveBeenCalled()
    })
  })

  const aliases: Alias[] = [
    {
      email: 'new@brave.com',
      note: 'New Alias',
      domains: undefined
    }, {
      email: 'existing@brave.com',
      note: 'Existing Alias',
      domains: ['brave.com']
    }
  ]

  for (const alias of aliases) {
    it('shows error message when creating or editing alias fails', async () => {
      mockEmailAliasesService.updateAlias = jest.fn().mockImplementation(
        () => Promise.resolve({ errorMessage: 'emailAliasesUpdateAliasError' }))

      render(
        <EmailAliasModal
          editing={alias.email === 'existing@brave.com'}
          editAlias={alias}
          mainEmail={mockEmail}
          aliasCount={0}
          onReturnToMain={mockOnReturnToMain}
          emailAliasesService={mockEmailAliasesService}
        />
      )

      // Wait for error message to appear. Create/Edit button should be enabled.
      const saveButton = screen.getByText(
        /emailAliasesCreateAliasButton|emailAliasesSaveAliasButton/)
      await waitFor(() => {
        expect(saveButton).toHaveAttribute('isdisabled', 'false')
      })

      await act(async () => {
        // Click save button
        saveButton.shadowRoot?.querySelector('button')?.click()
      })

      // Check that updateAlias was called and error message is displayed.
      await waitFor(() => {
        expect(mockEmailAliasesService.updateAlias).toHaveBeenCalled()
        expect(screen.getByText('emailAliasesUpdateAliasError'))
          .toBeInTheDocument()
      })
    })
  }

  it('shows error message when deleting alias fails', async () => {
    mockEmailAliasesService.deleteAlias = jest.fn().mockImplementation(
      () => Promise.resolve({ errorMessage: 'emailAliasesDeleteAliasError' }))

    render(
      <DeleteAliasModal
        onReturnToMain={mockOnReturnToMain}
        alias={mockAlias}
        emailAliasesService={mockEmailAliasesService}
      />
    )

    // Click delete button
    const deleteButton = screen.getByText('emailAliasesDeleteAliasButton')
    clickLeoButton(deleteButton)

    // Wait for error message to appear. Delete button should be enabled.
    await waitFor(() => {
      expect(mockEmailAliasesService.deleteAlias).toHaveBeenCalled()
      expect(screen.getByText('emailAliasesDeleteAliasError'))
        .toBeInTheDocument()
    })
  })

})
