// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render, screen, waitFor, act, fireEvent } from '@testing-library/react'
import { EmailAliasModal } from '../content/email_aliases_modal'
import { ViewState } from '../content/types'

import { clickLeoButton, localeRegex } from './test_utils'
import { EmailAliasesServiceInterface }
  from "gen/brave/components/email_aliases/email_aliases.mojom.m"

jest.mock('$web-common/locale', () => ({
  getLocale: (key: string) => {
    return key
  },
  formatMessage: (key: string, params: Record<string, string>) => {
    return key
  },
  formatLocale: (key: string, params: Record<string, string>) => {
    return key
  }
}))

// Mock the email aliases service
const mockEmailAliasesService: EmailAliasesServiceInterface = {
  logout: jest.fn(),
  updateAlias: jest.fn(),
  deleteAlias: jest.fn(),
  generateAlias: jest.fn(),
  showSettingsPage: jest.fn(),
  requestPrimaryEmailVerification: jest.fn(),
  cancelPrimaryEmailVerification: jest.fn(),
  addObserver: jest.fn(),
  removeObserver: jest.fn()
}

describe('EmailAliasModal', () => {
  const mockOnReturnToMain = jest.fn()
  const mockEmail = 'test@brave.com'

  beforeEach(() => {
    jest.clearAllMocks()
    mockEmailAliasesService.generateAlias = jest.fn()
      .mockResolvedValue({ alias: 'generated@brave.com' })
  })

  it('renders create mode correctly', async () => {
    render(
      <EmailAliasModal
        viewState={{ mode: 'Create' }}
        mainEmail={mockEmail}
        aliasCount={0}
        onReturnToMain={mockOnReturnToMain}
        emailAliasesService={mockEmailAliasesService}
      />
    )

    // Check title and description
    expect(screen.getByText(localeRegex('emailAliasesCreateAliasTitle')))
      .toBeInTheDocument()

    // Check that generate alias was called
    await waitFor(() => {
      expect(mockEmailAliasesService.generateAlias).toHaveBeenCalled()
    })
  })

  it('renders edit mode correctly', () => {
    const mockViewState: ViewState = {
      mode: 'Edit',
      alias: {
        email: 'existing@brave.com',
        note: 'Existing Alias',
        domains: ['brave.com']
      }
    }

    render(
      <EmailAliasModal
        viewState={mockViewState}
        mainEmail={mockEmail}
        aliasCount={0}
        onReturnToMain={mockOnReturnToMain}
        emailAliasesService={mockEmailAliasesService}
      />
    )

    // Check title
    expect(screen.getByText(localeRegex('emailAliasesEditAliasTitle')))
      .toBeInTheDocument()

    // Check that existing alias is displayed
    expect(screen.getByText(/existing@brave\.com/)).toBeInTheDocument()
    expect(screen.getByPlaceholderText(localeRegex(
      'emailAliasesEditNotePlaceholder'))).toHaveValue('Existing Alias')
  })

  it('handles alias creation', async () => {

    await act(async () => {
      render(
        <EmailAliasModal
        viewState={{ mode: 'Create' }}
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
      expect(screen.getByPlaceholderText(localeRegex(
        'emailAliasesEditNotePlaceholder'))).toBeInTheDocument()
      expect(screen.queryByTestId('loading-icon')).not.toBeInTheDocument()
      const generatedEmailContainer = screen.getByText(localeRegex(
        'emailAliasesAliasLabel')).closest('div')?.nextElementSibling
      expect(generatedEmailContainer).toHaveTextContent('generated@brave.com')
    })

    // Ensure the save button is enabled
    const saveButton = screen.getByText(localeRegex(
      'emailAliasesCreateAliasButton'))
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
        viewState={{ mode: 'Create' }}
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
    render(
      <EmailAliasModal
        viewState={{ mode: 'Create' }}
        mainEmail={mockEmail}
        aliasCount={5}
        onReturnToMain={mockOnReturnToMain}
        emailAliasesService={mockEmailAliasesService}
        bubble={true}
      />
    )

    // Wait for limit check
    await waitFor(() => {
      expect(screen.getByText(localeRegex('emailAliasesBubbleLimitReached')))
        .toBeInTheDocument()
    })
  })

  it('shows loading state while generating alias', async () => {
    // Mock generateAlias to take some time
    mockEmailAliasesService.generateAlias = jest.fn().mockImplementation(() => {
      return new Promise(
        resolve => setTimeout(() => resolve('new@brave.com'), 100))
    })

    render(
      <EmailAliasModal
        viewState={{ mode: 'Create' }}
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
    })
  })

  it('handles alias updates', async () => {
    const mockViewState: ViewState = {
      mode: 'Edit',
      alias: {
        email: 'existing@brave.com',
        note: 'Existing Alias',
        domains: ['brave.com']
      }
    }

    render(
      <EmailAliasModal
        viewState={mockViewState}
        mainEmail={mockEmail}
        aliasCount={0}
        onReturnToMain={mockOnReturnToMain}
        emailAliasesService={mockEmailAliasesService}
      />
    )

    // Wait for initial state to be set
    await waitFor(() => {
      expect(screen.getByText(/existing@brave\.com/)).toBeInTheDocument()
      expect(screen.getByPlaceholderText(localeRegex(
        'emailAliasesEditNotePlaceholder'))).toHaveValue('Existing Alias')
    })

    // Ensure the save button is enabled
    const saveButton = screen.getByText(localeRegex(
      'emailAliasesSaveAliasButton'))
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
})
