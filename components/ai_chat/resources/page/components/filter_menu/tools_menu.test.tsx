// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import '$test-utils/disable_custom_elements'

import * as React from 'react'

import { render } from '@testing-library/react'
import { ActionType } from 'gen/brave/components/ai_chat/core/common/mojom/common.mojom.m.js'
import ToolsMenu from './tools_menu'

describe('tools_menu', () => {
  const defaultProps: Parameters<typeof ToolsMenu>[0] = {
    handleClick: jest.fn(),
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

  it('should render', () => {
    const { getByText, container } = render(<ToolsMenu {...defaultProps} />)
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

  it('should hide subheadins when filtering', () => {
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
})
