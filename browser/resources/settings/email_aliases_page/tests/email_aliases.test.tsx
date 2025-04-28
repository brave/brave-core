// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render, screen, fireEvent, waitFor, act } from '@testing-library/react'
import { ManagePage } from '../email_aliases'
import { MappingService } from '../content/types'
import { localeRegex } from './test_utils'

// Mock the mapping service
const mockMappingService: MappingService = {
  getAccountEmail: jest.fn(),
  logout: jest.fn(),
  requestAccount: jest.fn(),
  getAliases: jest.fn(),
  createAlias: jest.fn(),
  updateAlias: jest.fn(),
  deleteAlias: jest.fn(),
  generateAlias: jest.fn(),
  onAccountReady: jest.fn(),
  cancelAccountRequest: jest.fn(),
  closeBubble: jest.fn(),
  fillField: jest.fn(),
  showSettingsPage: jest.fn()
}

describe('ManagePage', () => {
  beforeEach(() => {
    jest.clearAllMocks()
  })

  it('shows loading state initially', () => {
    render(<ManagePage mappingService={mockMappingService} />)
    expect(document.querySelector('[name="loading-spinner"]'))
      .toBeInTheDocument()
    expect(screen.getByText(localeRegex(
      'emailAliasesConnectingToBraveAccount'))).toBeInTheDocument()
  })

  it('shows sign up form when no email is available', async () => {
    mockMappingService.getAccountEmail = jest.fn().mockResolvedValue(null)

    await act(async () => {
      render(<ManagePage mappingService={mockMappingService} />)
    })

    await waitFor(() => {
      expect(screen.getByText(localeRegex('emailAliasesSignInOrCreateAccount')))
        .toBeInTheDocument()
      expect(screen.getByPlaceholderText(localeRegex(
        'emailAliasesEmailAddressPlaceholder'))).toBeInTheDocument()
    })
  })

  it('shows main view when email is available', async () => {
    const mockEmail = 'test@brave.com'
    const mockAliases = [
      {
        email: 'alias1@brave.com',
        note: 'Test Alias 1',
      }
    ]
    mockMappingService.getAccountEmail = jest.fn().mockResolvedValue(mockEmail)
    mockMappingService.getAliases = jest.fn().mockResolvedValue(mockAliases)

    await act(async () => {
      render(<ManagePage mappingService={mockMappingService} />)
    })

    await waitFor(() => {
      expect(screen.getByText(mockEmail)).toBeInTheDocument()
      expect(screen.getByText(/alias1@brave\.com/)).toBeInTheDocument()
      expect(screen.getByText(/Test Alias 1/)).toBeInTheDocument()
    })
  })

  it('handles sign up flow', async () => {
    const mockEmail = 'test@brave.com'
    mockMappingService.getAccountEmail = jest.fn().mockResolvedValue(null)
    mockMappingService.requestAccount = jest.fn()
    mockMappingService.onAccountReady = jest.fn().mockResolvedValue(true)
    mockMappingService.getAliases = jest.fn().mockResolvedValue([])

    await act(async () => {
      render(<ManagePage mappingService={mockMappingService} />)
    })

    // Wait for sign up form to be ready
    await waitFor(() => {
      const emailInput = document.querySelector('leo-input')
      const submitButton = document.querySelector('leo-button')
      expect(emailInput).toBeInTheDocument()
      expect(submitButton).toBeInTheDocument()
      expect(screen.getByText(localeRegex('emailAliasesSignInOrCreateAccount')))
        .toBeInTheDocument()
    })

    // Enter email and submit
    await act(async () => {
      const emailInput = document.querySelector('leo-input')
      if (emailInput) {
        emailInput.setAttribute('value', mockEmail)
      }
    })

    // Wait for the value to be set and stable
    await waitFor(() => {
      const emailInput = document.querySelector('leo-input')
      expect(emailInput).toHaveAttribute('value', mockEmail)
    })

    // Click submit button
    await act(async () => {
      const submitButton =
        screen.getByText(localeRegex('emailAliasesGetLoginLinkButton'))
      submitButton.shadowRoot?.querySelector('button')?.click()
    })

    // Wait for main view to be shown
    await waitFor(() => {
      expect(screen.getByText(localeRegex(
        'emailAliasesConnectingToBraveAccount'))).toBeInTheDocument()
    })
  })

  it('handles logout', async () => {
    const mockEmail = 'test@brave.com'
    mockMappingService.getAccountEmail = jest.fn().mockResolvedValue(mockEmail)
    mockMappingService.getAliases = jest.fn().mockResolvedValue([])

    await act(async () => {
      render(<ManagePage mappingService={mockMappingService} />)
    })

    // Wait for main view
    await waitFor(() => {
      expect(screen.getByText(mockEmail)).toBeInTheDocument()
    })

    // Click logout button
    const logoutButton = screen.getByText(localeRegex('emailAliasesSignOut'))
    await act(async () => {
      fireEvent.click(logoutButton)
    })

    // Check that logout was called
    expect(mockMappingService.logout).toHaveBeenCalled()

    // Check that we're back to sign up form
    await waitFor(() => {
      expect(screen.getByText(localeRegex('emailAliasesSignInOrCreateAccount')))
        .toBeInTheDocument()
    })
  })
})
