// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import '$test-utils/disable_custom_elements'

import * as React from 'react'

import { render, screen, fireEvent } from '@testing-library/react'
import { ActionType } from 'gen/brave/components/ai_chat/core/common/mojom/ai_chat.mojom.m.js'
import ToolsMenu from './tools_menu'
import { AIChatReactContext } from '../../state/ai_chat_context'
import * as Mojom from '../../../common/mojom'

// Mock locale function
jest.mock('$web-common/locale', () => ({
  getLocale: (key: string) => {
    const mockStrings: { [key: string]: string } = {
      CHAT_UI_SMART_MODES_GROUP: 'Smart Modes',
    }
    return mockStrings[key] || key
  },
}))

describe('tools_menu', () => {
  const mockSmartModes: Mojom.SmartMode[] = [
    {
      id: 'smart-mode-1',
      shortcut: 'translate',
      prompt: 'Translate the following text',
      model: 'claude-3-haiku',
    },
    {
      id: 'smart-mode-2',
      shortcut: 'explain',
      prompt: 'Explain this concept',
      model: null,
    },
  ]

  const defaultProps: Parameters<typeof ToolsMenu>[0] = {
    handleClick: jest.fn(),
    handleSmartModeClick: jest.fn(),
    handleSmartModeEdit: jest.fn(),
    categories: [
      {
        category: 'category 1',
        entries: [
          {
            subheading: 'subheading 1',
            details: undefined,
          },
          {
            details: {
              label: 'entry 1',
              type: ActionType.CASUALIZE,
            },
            subheading: undefined,
          },
          {
            details: {
              label: 'entry 2',
              type: ActionType.EXPLAIN,
            },
            subheading: undefined,
          },
        ],
      },
      {
        category: 'category 2',
        entries: [
          {
            subheading: 'subheading 2',
            details: undefined,
          },
          {
            details: {
              label: 'entry 3',
              type: ActionType.FUNNY_TONE,
            },
            subheading: undefined,
          },
        ],
      },
    ],
    isOpen: true,
    setIsOpen: jest.fn(),
    query: null,
  }

  const renderWithContext = (
    smartModes: Mojom.SmartMode[] = mockSmartModes,
    props = defaultProps,
  ) => {
    const aiChatContext = { smartModes }
    return render(
      <AIChatReactContext.Provider value={aiChatContext as any}>
        <ToolsMenu {...props} />
      </AIChatReactContext.Provider>,
    )
  }

  beforeEach(() => {
    jest.clearAllMocks()
  })

  describe('without smart modes', () => {
    it('should render categories without smart modes', () => {
      const { getByText, container } = renderWithContext([])
      expect(getByText('category 1')).toBeInTheDocument()
      expect(getByText('category 2')).toBeInTheDocument()
      expect(getByText('subheading 1')).toBeInTheDocument()
      expect(getByText('subheading 2')).toBeInTheDocument()
      expect(getByText('entry 1')).toBeInTheDocument()
      expect(getByText('entry 2')).toBeInTheDocument()
      expect(getByText('entry 3')).toBeInTheDocument()

      expect(container.querySelector('leo-buttonmenu')).toHaveProperty(
        'isOpen',
        true,
      )
    })

    it('should hide subheadings when filtering', () => {
      const { queryByText } = renderWithContext([], {
        ...defaultProps,
        query: 'e',
      })
      expect(queryByText('subheading 1')).not.toBeInTheDocument()
      expect(queryByText('subheading 2')).not.toBeInTheDocument()
    })

    it('should hide categories with no matches', () => {
      const { queryByText } = renderWithContext([], {
        ...defaultProps,
        query: 'entry 3',
      })
      expect(queryByText('category 1')).not.toBeInTheDocument()
      expect(queryByText('category 2')).toBeInTheDocument()
    })
  })

  describe('with smart modes', () => {
    it('should render smart modes category first', () => {
      const { getByText } = renderWithContext()
      expect(getByText('Smart Modes')).toBeInTheDocument()
      expect(getByText('translate')).toBeInTheDocument()
      expect(getByText('explain')).toBeInTheDocument()
    })

    it('should render smart mode entries with edit buttons', () => {
      const { container } = renderWithContext()

      // Check that smart mode shortcuts are rendered
      expect(screen.getByText('translate')).toBeInTheDocument()
      expect(screen.getByText('explain')).toBeInTheDocument()

      // Check that edit buttons are present using leo-button selector
      const editButtons = container.querySelectorAll('leo-button')
      expect(editButtons.length).toBeGreaterThanOrEqual(2)
    })

    it('should call handleSmartModeClick when smart mode is clicked', () => {
      const mockHandleSmartModeClick = jest.fn()
      renderWithContext(mockSmartModes, {
        ...defaultProps,
        handleSmartModeClick: mockHandleSmartModeClick,
      })

      const translateItem = screen
        .getByText('translate')
        .closest('leo-menu-item')
      fireEvent.click(translateItem!)

      expect(mockHandleSmartModeClick).toHaveBeenCalledWith(mockSmartModes[0])
    })

    it('should call handleSmartModeEdit when edit button is clicked', () => {
      const mockHandleSmartModeEdit = jest.fn()
      const { container } = renderWithContext(mockSmartModes, {
        ...defaultProps,
        handleSmartModeEdit: mockHandleSmartModeEdit,
      })

      const editButtons = container.querySelectorAll('leo-button')
      const firstEditButton = editButtons[0] as HTMLElement
      fireEvent.click(firstEditButton)

      expect(mockHandleSmartModeEdit).toHaveBeenCalledWith(mockSmartModes[0])
    })

    it('should filter smart modes by shortcut', () => {
      const { queryByText } = renderWithContext(mockSmartModes, {
        ...defaultProps,
        query: 'trans',
      })

      expect(queryByText('translate')).toBeInTheDocument()
      expect(queryByText('explain')).not.toBeInTheDocument()
    })

    it('should show smart modes category when others hidden', () => {
      const { getByText, queryByText } = renderWithContext(mockSmartModes, {
        ...defaultProps,
        query: 'translate',
      })

      expect(getByText('Smart Modes')).toBeInTheDocument()
      expect(getByText('translate')).toBeInTheDocument()
      expect(queryByText('category 1')).not.toBeInTheDocument()
      expect(queryByText('category 2')).not.toBeInTheDocument()
    })

    it('should not render smart modes section when no handlers', () => {
      const propsWithoutHandlers = {
        ...defaultProps,
        handleSmartModeClick: undefined,
        handleSmartModeEdit: undefined,
      }
      const { queryByText } = renderWithContext(
        mockSmartModes,
        propsWithoutHandlers,
      )

      expect(queryByText('Smart Modes')).not.toBeInTheDocument()
      expect(queryByText('translate')).not.toBeInTheDocument()
    })

    it('should render combined categories in correct order', () => {
      const { container } = renderWithContext()

      const menuItems = Array.from(container.querySelectorAll('leo-menu-item'))
      const categoryHeaders = Array.from(
        container.querySelectorAll('.menuSectionTitle, .menuSubtitle'),
      )

      // Smart Modes should appear first, then other categories
      expect(categoryHeaders[0]).toHaveTextContent('Smart Modes')
      expect(menuItems[0]).toHaveTextContent('translate')
      expect(menuItems[1]).toHaveTextContent('explain')
    })

    it('should handle empty smart modes array', () => {
      const { queryByText } = renderWithContext([])

      expect(queryByText('Smart Modes')).not.toBeInTheDocument()
      expect(queryByText('translate')).not.toBeInTheDocument()
      expect(queryByText('explain')).not.toBeInTheDocument()
    })
  })
})
