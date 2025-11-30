// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import '$test-utils/disable_custom_elements'

import { act, render, waitFor, fireEvent } from '@testing-library/react'
import FilterMenu, { Props } from './filter_menu'
import * as React from 'react'
import { matches } from './query'

describe('filter_menu', () => {
  const defaultProps: Props<string> = {
    isOpen: true,
    setIsOpen: jest.fn(),
    query: null,
    categories: [
      {
        category: 'test',
        entries: ['hello', 'world'],
      },
    ],
    matchesQuery: (q, item) => matches(q, item),
    children: () => null,
  }

  it('should be hidden when isOpen is false', () => {
    const { container } = render(
      <FilterMenu
        {...defaultProps}
        isOpen={false}
      >
        {(item) => <leo-menu-item>{item}</leo-menu-item>}
      </FilterMenu>,
    )
    expect(container.querySelector('leo-buttonmenu')).toHaveProperty(
      'isOpen',
      false,
    )
  })

  it('should be shown when isOpen is true', () => {
    const { container } = render(
      <FilterMenu
        {...defaultProps}
        isOpen={true}
      >
        {(item) => <leo-menu-item>{item}</leo-menu-item>}
      </FilterMenu>,
    )
    expect(container.querySelector('leo-buttonmenu')).toHaveProperty(
      'isOpen',
      true,
    )
  })

  it('should open when a query is set', async () => {
    render(
      <FilterMenu
        {...defaultProps}
        isOpen={false}
        query={'hello'}
      >
        {(item) => <leo-menu-item>{item}</leo-menu-item>}
      </FilterMenu>,
    )

    await waitFor(async () => {
      expect(defaultProps.setIsOpen).toHaveBeenCalledWith(true)
    })
  })

  it('should open for empty queries', async () => {
    render(
      <FilterMenu
        {...defaultProps}
        isOpen={false}
        query=''
      >
        {(item) => <leo-menu-item>{item}</leo-menu-item>}
      </FilterMenu>,
    )

    await waitFor(async () => {
      expect(defaultProps.setIsOpen).toHaveBeenCalledWith(true)
    })
  })

  it('should close when query goes null', async () => {
    render(
      <FilterMenu
        {...defaultProps}
        isOpen={true}
        query={null}
      >
        {(item) => <leo-menu-item>{item}</leo-menu-item>}
      </FilterMenu>,
    )

    await waitFor(async () => {
      expect(defaultProps.setIsOpen).toHaveBeenCalledWith(false)
    })
  })

  it('should render children', async () => {
    const { queryByText } = render(
      <FilterMenu
        {...defaultProps}
        isOpen={true}
        query={null}
      >
        {(item) => <leo-menu-item>{item}</leo-menu-item>}
      </FilterMenu>,
    )

    expect(queryByText('hello')).toBeInTheDocument()
    expect(queryByText('world')).toBeInTheDocument()
  })

  it('should filter children', async () => {
    const { queryByText } = render(
      <FilterMenu
        {...defaultProps}
        isOpen={true}
        query={'hell'}
      >
        {(item) => <leo-menu-item>{item}</leo-menu-item>}
      </FilterMenu>,
    )

    expect(queryByText('hello')).toBeInTheDocument()
    expect(queryByText('world')).not.toBeInTheDocument()
  })

  it('should click first menu-item when enter is pressed and nothing is focused', async () => {
    const click = jest.fn()
    render(
      <FilterMenu
        {...defaultProps}
        isOpen={true}
        query={''}
      >
        {(item) => (
          <leo-menu-item
            tabindex={0}
            onClick={() => click(item)}
          >
            {item}
          </leo-menu-item>
        )}
      </FilterMenu>,
    )

    await act(() => {
      fireEvent.keyDown(document, { key: 'Enter' })
    })

    expect(click).toHaveBeenCalledWith('hello')
  })

  it('should click focused menu-item when enter is pressed', async () => {
    const click = jest.fn()
    const { getByText } = render(
      <FilterMenu
        {...defaultProps}
        isOpen={true}
        query={''}
      >
        {(item) => (
          <leo-menu-item
            tabindex={0}
            onClick={() => click(item)}
          >
            {item}
          </leo-menu-item>
        )}
      </FilterMenu>,
    )

    await act(() => {
      getByText('world').focus()
    })

    await act(() => {
      fireEvent.keyDown(document, { key: 'Enter' })
    })

    expect(click).toHaveBeenCalledWith('world')
  })

  it('should filter out categories with no matches', async () => {
    const click = jest.fn()
    const { queryByText } = render(
      <FilterMenu
        {...defaultProps}
        categories={[
          { category: 'should be hidden', entries: ['one', 'two', 'three'] },
          { category: 'should be shown', entries: ['hello', 'world'] },
        ]}
        query={'hell'}
      >
        {(item) => (
          <leo-menu-item
            tabindex={0}
            onClick={() => click(item)}
          >
            {item}
          </leo-menu-item>
        )}
      </FilterMenu>,
    )

    expect(queryByText('should be shown')).toBeInTheDocument()
    expect(queryByText('should be hidden')).not.toBeInTheDocument()
  })
})
