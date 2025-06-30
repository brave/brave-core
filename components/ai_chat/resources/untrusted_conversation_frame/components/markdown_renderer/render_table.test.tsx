// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import '@testing-library/jest-dom'
import { render, screen } from '@testing-library/react'
import MarkdownRenderer from '.'

// Mock the context to avoid dependency issues
jest.mock('../../untrusted_conversation_context', () => ({
  useUntrustedConversationContext: () => ({
    parentUiFrame: {
      userRequestedOpenGeneratedUrl: jest.fn(),
    },
  }),
}))

describe('Table Rendering', () => {
  const renderMarkdown = (text: string, props = {}) => {
    return render(
      <MarkdownRenderer
        text={text}
        shouldShowTextCursor={false}
        {...props}
      />,
    )
  }

  describe('Basic Table Rendering', () => {
    test('renders a simple table with headers and data', () => {
      const markdown = `
| Name | Age | City |
|------|-----|------|
| John | 25  | NYC  |
| Jane | 30  | LA   |
      `.trim()

      renderMarkdown(markdown)

      // Check table wrapper
      const tableWrapper = document.querySelector('.tableWrapper')
      expect(tableWrapper).toBeInTheDocument()

      // Check table element
      const table = document.querySelector('table')
      expect(table).toBeInTheDocument()
      expect(table).toHaveClass('table')

      // Check headers
      expect(screen.getByText('Name')).toBeInTheDocument()
      expect(screen.getByText('Age')).toBeInTheDocument()
      expect(screen.getByText('City')).toBeInTheDocument()

      // Check data cells
      expect(screen.getByText('John')).toBeInTheDocument()
      expect(screen.getByText('25')).toBeInTheDocument()
      expect(screen.getByText('NYC')).toBeInTheDocument()
      expect(screen.getByText('Jane')).toBeInTheDocument()
      expect(screen.getByText('30')).toBeInTheDocument()
      expect(screen.getByText('LA')).toBeInTheDocument()
    })

    test('renders table with proper CSS classes', () => {
      const markdown = `
| Header 1 | Header 2 |
|----------|----------|
| Data 1   | Data 2   |
      `.trim()

      renderMarkdown(markdown)

      // Check table structure classes
      expect(document.querySelector('thead')).toHaveClass('tableHead')
      expect(document.querySelector('tbody')).toHaveClass('tableBody')
      // header + data row
      expect(document.querySelectorAll('tr')).toHaveLength(2)
      expect(document.querySelectorAll('th')).toHaveLength(2)
      expect(document.querySelectorAll('td')).toHaveLength(2)

      // Check individual element classes
      document.querySelectorAll('th').forEach((th) => {
        expect(th).toHaveClass('tableHeader')
      })

      document.querySelectorAll('td').forEach((td) => {
        expect(td).toHaveClass('tableCell')
      })

      document.querySelectorAll('tr').forEach((tr) => {
        expect(tr).toHaveClass('tableRow')
      })
    })
  })

  describe('Header Tracking and Data Labels', () => {
    test('cells have data-label attributes based on headers', () => {
      const markdown = `
| Name | Age | Location |
|------|-----|----------|
| John | 25  | NYC      |
| Jane | 30  | LA       |
      `.trim()

      renderMarkdown(markdown)

      const cells = document.querySelectorAll('td')
      expect(cells).toHaveLength(6)

      // First row cells should have data-label from first header
      expect(cells[0]).toHaveAttribute('data-label', 'Name')
      expect(cells[1]).toHaveAttribute('data-label', 'Age')
      expect(cells[2]).toHaveAttribute('data-label', 'Location')
    })

    test('handles empty header cells gracefully', () => {
      const markdown = `
| Name | | Location |
|------|--|----------|
| John | 25 | NYC    |
      `.trim()

      renderMarkdown(markdown)

      const cells = document.querySelectorAll('td')
      expect(cells[0]).toHaveAttribute('data-label', 'Name')
      // Empty header not stored, so gets next header
      expect(cells[1]).toHaveAttribute('data-label', 'Location')
      // No more headers
      expect(cells[2]).toHaveAttribute('data-label', '')
    })
  })

  describe('Complex Table Structures', () => {
    test('renders table with links in cells', () => {
      const markdown = `
| Name | Website |
|------|---------|
| John | [Brave](https://brave.com) |
      `.trim()

      renderMarkdown(markdown, { allowedLinks: ['https://brave.com'] })

      const link = screen.getByText('Brave')
      expect(link).toBeInTheDocument()
      expect(link.tagName).toBe('A')
      expect(link).toHaveClass('conversationLink')
    })
  })

  describe('Edge Cases', () => {
    test('handles empty table gracefully', () => {
      const markdown = `
| | |
|--|--|
| | |
      `.trim()

      renderMarkdown(markdown)

      const table = document.querySelector('table')
      expect(table).toBeInTheDocument()

      const cells = document.querySelectorAll('td')
      expect(cells).toHaveLength(2)

      cells.forEach((cell) => {
        expect(cell).toHaveAttribute('data-label', '')
      })
    })

    test('handles table with only headers', () => {
      const markdown = `
| Header 1 | Header 2 |
|----------|----------|
      `.trim()

      renderMarkdown(markdown)

      const table = document.querySelector('table')
      expect(table).toBeInTheDocument()

      const headers = document.querySelectorAll('th')
      expect(headers).toHaveLength(2)

      const cells = document.querySelectorAll('td')
      expect(cells).toHaveLength(0)
    })

    test('renders multiple tables correctly', () => {
      const markdown = `
| Table 1 | Data |
|---------|------|
| Row 1   | Data |

| Table 2 | Data |
|---------|------|
| Row 1   | Data |
      `.trim()

      renderMarkdown(markdown)

      const tables = document.querySelectorAll('table')
      expect(tables).toHaveLength(2)

      const tableWrappers = document.querySelectorAll('.tableWrapper')
      expect(tableWrappers).toHaveLength(2)

      // Each table should have its own structure
      tables.forEach((table) => {
        expect(table.querySelector('thead')).toBeInTheDocument()
        expect(table.querySelector('tbody')).toBeInTheDocument()
      })
    })
  })
})
