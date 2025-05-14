// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render, screen, fireEvent } from '@testing-library/react'
import { AliasList } from '../content/email_aliases_list'
import { getLocale } from '$web-common/locale'
import {
  Alias,
  EmailAliasesServiceInterface
} from 'gen/brave/components/email_aliases/email_aliases.mojom.m'

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

// Mock the clipboard API
Object.assign(navigator, {
  clipboard: {
    writeText: jest.fn(),
  },
})

// Mock the email aliases service
const mockEmailAliasesService: EmailAliasesServiceInterface = {
  logout: jest.fn(),
  updateAlias: jest.fn(),
  deleteAlias: jest.fn(),
  generateAlias: jest.fn(),
  requestAuthentication: jest.fn(),
  cancelAuthentication: jest.fn(),
  addObserver: jest.fn(),
  removeObserver: jest.fn()
}

describe('AliasList', () => {
  const mockAliases: Alias[] = [
    {
      email: 'test1@brave.com',
      note: 'Test Alias 1',
      domains: ['brave.com']
    },
    {
      email: 'test2@brave.com',
      note: 'Test Alias 2',
      domains: ['brave.com']
    }
  ]

  const mockOnEditStateChange = jest.fn()

  beforeEach(() => {
    jest.clearAllMocks()
  })

  it('renders the alias list', () => {
    render(
      <AliasList
        aliases={mockAliases}
        onEditStateChange={mockOnEditStateChange}
        emailAliasesService={mockEmailAliasesService}
      />
    )

    // Check if alias information is displayed correctly
    expect(screen.getByText(/test1@brave\.com/)).toBeInTheDocument()
    expect(screen.getByText(/Test Alias 1/)).toBeInTheDocument()
    expect(screen.getByText(/test2@brave\.com/)).toBeInTheDocument()
    expect(screen.getByText(/Test Alias 2/)).toBeInTheDocument()
  })

  it('handles copy functionality', () => {
    render(
      <AliasList
        aliases={mockAliases}
        onEditStateChange={mockOnEditStateChange}
        emailAliasesService={mockEmailAliasesService}
      />
    )

    // Click copy button
    const copyButtons = screen.getAllByTestId('copy-toast')
    fireEvent.click(copyButtons[0])

    // Check if clipboard API was called
    expect(navigator.clipboard.writeText)
      .toHaveBeenCalledWith('test1@brave.com')
  })

  it('copies email when clicking alias label', () => {
    render(
      <AliasList
        aliases={mockAliases}
        onEditStateChange={mockOnEditStateChange}
        emailAliasesService={mockEmailAliasesService}
      />
    )

    // Click the alias label
    const aliasLabel = screen.getByText(/test1@brave\.com/)
    fireEvent.click(aliasLabel)

    // Check if clipboard API was called
    expect(navigator.clipboard.writeText)
      .toHaveBeenCalledWith('test1@brave.com')
  })

  it('disables create button when max aliases reached', () => {
    const maxAliases = Array(10).fill(null).map((_, i) => ({
      email: `test${i}@brave.com`,
      note: `Test Alias ${i}`,
      domains: ['brave.com']
    }))

    render(
      <AliasList
        aliases={maxAliases}
        onEditStateChange={mockOnEditStateChange}
        emailAliasesService={mockEmailAliasesService}
      />
    )

    // Check if create button is disabled
    const createButton = screen.getByText(getLocale(
      'emailAliasesCreateAliasLabel'))
    expect(createButton.closest('leo-button'))
      .toHaveAttribute('isdisabled', 'true')
  })

  it('handles delete functionality', async () => {
    render(
      <AliasList
        aliases={mockAliases}
        onEditStateChange={mockOnEditStateChange}
        emailAliasesService={mockEmailAliasesService}
      />
    )

    // Click delete button
    const deleteButtons = screen.getAllByText(getLocale('emailAliasesDelete'))
    fireEvent.click(deleteButtons[0])

    // Check if service was called
    expect(mockEmailAliasesService.deleteAlias)
      .toHaveBeenCalledWith('test1@brave.com')
  })

  it('handles edit functionality', () => {
    render(
      <AliasList
        aliases={mockAliases}
        onEditStateChange={mockOnEditStateChange}
        emailAliasesService={mockEmailAliasesService}
      />
    )

    // Click edit button
    const editButtons = screen.getAllByText(getLocale('emailAliasesEdit'))
    fireEvent.click(editButtons[0])

    // Check if view change was called with correct state
    expect(mockOnEditStateChange).toHaveBeenCalledWith({
      mode: 'Edit',
      alias: mockAliases[0]
    })
  })
})
