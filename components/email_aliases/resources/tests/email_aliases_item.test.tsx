// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render, screen, fireEvent } from '@testing-library/react'
import { AliasItem } from '../content/email_aliases_item'
import { Alias } from 'gen/brave/components/email_aliases/email_aliases.mojom.m'

// Mock the callback functions
const mockOnEdit = jest.fn()
const mockOnDelete = jest.fn()

// Mock the clipboard API
Object.assign(navigator, {
  clipboard: {
    writeText: jest.fn(),
  },
})

describe('AliasItem', () => {
  const mockAlias: Alias = {
    email: 'test1@brave.com',
    note: 'Test Alias 1',
    domains: ['brave.com'],
  }

  beforeEach(() => {
    jest.clearAllMocks()
  })

  it('renders the alias item', () => {
    render(
      <AliasItem
        alias={mockAlias}
        onEdit={mockOnEdit}
        onDelete={mockOnDelete}
      />,
    )

    // Check if alias information is displayed correctly
    expect(screen.getByText(/test1@brave\.com/)).toBeInTheDocument()
    expect(screen.getByText(/Test Alias 1/)).toBeInTheDocument()
  })

  it('handles copy functionality', () => {
    render(
      <AliasItem
        alias={mockAlias}
        onEdit={mockOnEdit}
        onDelete={mockOnDelete}
      />,
    )

    // Click copy button
    const copyButtons = screen.getAllByTestId('copy-toast')
    fireEvent.click(copyButtons[0])

    // Check if clipboard API was called
    expect(navigator.clipboard.writeText).toHaveBeenCalledWith(
      'test1@brave.com',
    )
  })

  it('copies email when clicking alias label', () => {
    render(
      <AliasItem
        alias={mockAlias}
        onEdit={mockOnEdit}
        onDelete={mockOnDelete}
      />,
    )

    // Click the alias label
    const aliasLabel = screen.getByText(/test1@brave\.com/)
    fireEvent.click(aliasLabel)

    // Check if clipboard API was called
    expect(navigator.clipboard.writeText).toHaveBeenCalledWith(
      'test1@brave.com',
    )
  })

  it('handles delete functionality', async () => {
    render(
      <AliasItem
        alias={mockAlias}
        onEdit={mockOnEdit}
        onDelete={mockOnDelete}
      />,
    )

    // Click delete button
    const deleteButton = screen.getByText(S.SETTINGS_EMAIL_ALIASES_DELETE)
    fireEvent.click(deleteButton)

    // Check if callback was called
    expect(mockOnDelete).toHaveBeenCalled()
  })

  it('handles edit functionality', async () => {
    render(
      <AliasItem
        alias={mockAlias}
        onEdit={mockOnEdit}
        onDelete={mockOnDelete}
      />,
    )

    // Click edit button
    const editButton = screen.getByText(S.SETTINGS_EMAIL_ALIASES_EDIT)
    fireEvent.click(editButton)

    // Check if callback was called
    expect(mockOnEdit).toHaveBeenCalled()
  })
})
