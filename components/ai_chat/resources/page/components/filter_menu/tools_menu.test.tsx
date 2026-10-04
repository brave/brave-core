// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import '$test-utils/disable_custom_elements'

import * as React from 'react'

import { act, render } from '@testing-library/react'
import { ActionType } from 'gen/brave/components/ai_chat/core/common/mojom/common.mojom.m.js'
import ToolsMenu from './tools_menu'
import { ActionEntry, Skill } from '../../../common/mojom'

describe('tools_menu', () => {
  const defaultProps: Parameters<typeof ToolsMenu>[0] = {
    handleClick: jest.fn(),
    handleEditClick: jest.fn(),
    handleNewSkillClick: jest.fn(),
    categories: [
      {
        category: 'skill subheading',
        entries: [
          {
            id: 'skill 1',
            shortcut: 'skill 1',
          } as Skill,
          {
            id: 'skill 2',
            shortcut: 'skill 2',
          } as Skill,
        ],
      },
      {
        category: 'category 1',
        entries: [
          {
            subheading: 'subheading 1',
            details: undefined,
          } as ActionEntry,
          {
            details: {
              label: 'entry 1',
              type: ActionType.CASUALIZE,
            },
            subheading: undefined,
          } as ActionEntry,
          {
            details: {
              label: 'entry 2',
              type: ActionType.EXPLAIN,
            },
            subheading: undefined,
          } as ActionEntry,
        ],
      },
      {
        category: 'category 2',
        entries: [
          {
            subheading: 'subheading 2',
            details: undefined,
          } as ActionEntry,
          {
            details: {
              label: 'entry 3',
              type: ActionType.FUNNY_TONE,
            },
            subheading: undefined,
          } as ActionEntry,
        ],
      },
    ],
    isOpen: true,
    setIsOpen: jest.fn(),
    query: null,
  }

  it('should render', () => {
    const { getByText, container } = render(<ToolsMenu {...defaultProps} />)
    expect(getByText('skill subheading')).toBeInTheDocument()
    expect(getByText('skill 1')).toBeInTheDocument()
    expect(getByText('skill 2')).toBeInTheDocument()
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
    const { queryByText } = render(
      <ToolsMenu
        {...defaultProps}
        query='e'
      />,
    )
    expect(queryByText('subheading 1')).not.toBeInTheDocument()
    expect(queryByText('subheading 2')).not.toBeInTheDocument()
  })

  it('should hide categories with no matches', () => {
    const { queryByText } = render(
      <ToolsMenu
        {...defaultProps}
        query='entry 3'
      />,
    )
    expect(queryByText('category 1')).not.toBeInTheDocument()
    expect(queryByText('category 2')).toBeInTheDocument()
  })

  it('should show skills when filtering', async () => {
    const { container } = await act(async () =>
      render(
        <ToolsMenu
          {...defaultProps}
          query='skill'
        />,
      ),
    )
    const matches = Array.from(container.querySelectorAll('.matchedText'))
    expect(matches).toHaveLength(2)
    expect(matches[0]).toHaveTextContent('skill')
    expect(matches[1]).toHaveTextContent('skill')
  })

  describe('tools menu click handling', () => {
    it('should call handleClick when clicking on action entries', () => {
      const handleClick = jest.fn()
      const { getByText } = render(
        <ToolsMenu
          {...defaultProps}
          handleClick={handleClick}
        />,
      )

      // Click on entry 1
      getByText('entry 1').click()
      expect(handleClick).toHaveBeenCalledWith({
        details: {
          label: 'entry 1',
          type: ActionType.CASUALIZE,
        },
        subheading: undefined,
      })

      // Click on entry 2
      getByText('entry 2').click()
      expect(handleClick).toHaveBeenCalledWith({
        details: {
          label: 'entry 2',
          type: ActionType.EXPLAIN,
        },
        subheading: undefined,
      })

      expect(handleClick).toHaveBeenCalledTimes(2)
    })

    it('should call handleClick when clicking on skills', () => {
      const handleClick = jest.fn()
      const { getByText } = render(
        <ToolsMenu
          {...defaultProps}
          handleClick={handleClick}
        />,
      )

      // Click on skill 1
      getByText('skill 1').click()
      expect(handleClick).toHaveBeenCalledWith({
        id: 'skill 1',
        shortcut: 'skill 1',
      })

      // Click on skill 2
      getByText('skill 2').click()
      expect(handleClick).toHaveBeenCalledWith({
        id: 'skill 2',
        shortcut: 'skill 2',
      })

      expect(handleClick).toHaveBeenCalledTimes(2)
    })

    it('should call handleEditClick when clicking edit', () => {
      const handleEditClick = jest.fn()
      const { container } = render(
        <ToolsMenu
          {...defaultProps}
          handleEditClick={handleEditClick}
        />,
      )

      // Find and click the edit button for skill 1
      const editButton1 = container.querySelectorAll(
        'leo-button[class*="editButton"]',
      )[0] as HTMLElement
      expect(editButton1).toBeInTheDocument()
      editButton1.click()

      expect(handleEditClick).toHaveBeenCalledWith({
        id: 'skill 1',
        shortcut: 'skill 1',
      })

      // Find and click the edit button for skill 2
      const editButton2 = container.querySelectorAll(
        'leo-button[class*="editButton"]',
      )[1] as HTMLElement
      expect(editButton2).toBeInTheDocument()
      editButton2.click()

      expect(handleEditClick).toHaveBeenCalledWith({
        id: 'skill 2',
        shortcut: 'skill 2',
      })

      expect(handleEditClick).toHaveBeenCalledTimes(2)
    })
  })
})
