// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render, screen, waitFor, act } from '@testing-library/react'
import { EmailAliasModal, EditState, EditMode }
  from '../content/email_aliases_modal'

import { clickLeoButton } from './test_utils'
import { EmailAliasesServiceInterface, GenerateAliasResult }
  from "gen/brave/components/email_aliases/email_aliases.mojom.m"

jest.mock('$web-common/locale', () => ({
  getLocale: (key: string) => {
    return key
  },
  formatMessage: (key: string) => {
    return key
  },
  formatLocale: (key: string) => {
    return key
  }
}))

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

  beforeEach(() => {
    jest.clearAllMocks()
    mockEmailAliasesService.updateAlias = jest.fn().mockResolvedValue(
      Promise.resolve({ errorMessage: null }))
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
        editState={{ mode: 'Create' }}
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
    const mockEditState: EditState = {
      mode: 'Edit',
      alias: {
        email: 'existing@brave.com',
        note: 'Existing Alias',
        domains: ['brave.com']
      }
    }

    render(
      <EmailAliasModal
        editState={mockEditState}
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

  it('handles alias creation', async () => {
    await act(async () => {
      render(
        <EmailAliasModal
        editState={{ mode: 'Create' }}
        mainEmail={mockEmail}
        aliasCount={0}
        onReturnToMain={mockOnReturnToMain}
        emailAliasesService={mockEmailAliasesService}
      />)
    })

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
        editState={{ mode: 'Create' }}
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
    await act(async () => {
      clickLeoButton(regenerateButton)
    })

    // Check that generateAlias was called again
    await waitFor(() => {
      expect(mockEmailAliasesService.generateAlias).toHaveBeenCalledTimes(2)
    })
  })

  it('shows limit reached message in bubble mode', async () => {
    await act(async () => {
      render(
        <EmailAliasModal
          editState={{ mode: 'Create' }}
          mainEmail={mockEmail}
          aliasCount={5}
          onReturnToMain={mockOnReturnToMain}
        emailAliasesService={mockEmailAliasesService}
          bubble={true}
        />
      )
    })

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
        editState={{ mode: 'Create' }}
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
        editState={{ mode: 'Create' }}
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
    const mockEditState: EditState = {
      mode: 'Edit',
      alias: {
        email: 'existing@brave.com',
        note: 'Existing Alias',
        domains: ['brave.com']
      }
    }

    render(
      <EmailAliasModal
        editState={mockEditState}
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

  const states: EditState[] = [{
    mode: 'Create' as EditMode,
    alias: {
      email: 'new@brave.com',
      note: 'New Alias',
      domains: undefined
    }
  }, {
    mode: 'Edit' as EditMode,
    alias: {
      email: 'existing@brave.com',
      note: 'Existing Alias',
      domains: ['brave.com']
    }
  }]

  for (const state of states) {
    it('shows error message when creating or editing alias fails', async () => {
      mockEmailAliasesService.updateAlias = jest.fn().mockImplementation(
        () => Promise.resolve({ errorMessage: 'emailAliasesUpdateAliasError' }))

      render(
        <EmailAliasModal
          editState={state}
          mainEmail={mockEmail}
          aliasCount={0}
          onReturnToMain={mockOnReturnToMain}
          emailAliasesService={mockEmailAliasesService}
        />
      )

      // Wait for error message to appear. Create button should be disabled.
      const saveButton = screen.getByText(
        /emailAliasesCreateAliasButton|emailAliasesSaveAliasButton/)
      await waitFor(() => {
        expect(saveButton).toHaveAttribute('isdisabled', 'false')
      })

      await act(async () => {
        // Click save button
        saveButton.shadowRoot?.querySelector('button')?.click()
      })

      // Check that updateAlias was called
      await waitFor(() => {
        expect(mockEmailAliasesService.updateAlias).toHaveBeenCalled()
      })

      // Wait for error message to be displayed. Create button should be
      // disabled.
      await waitFor(() => {
        expect(screen.getByText('emailAliasesUpdateAliasError'))
          .toBeInTheDocument()
      })
    })
  }

})
