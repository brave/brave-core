// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render } from '@testing-library/react'
import { SearchBar } from '.'

describe('wallet search bar', () => {
  it('renders search bar with search icon and search input', () => {
    const { container } = render(
      <SearchBar
        placeholder='Search'
        value='test'
        action={() => {}}
        isV2={true}
      />
    )

    // Test search input
    const searchInput = container.querySelector('[data-key="search-input"]')
    expect(searchInput).toBeInTheDocument()
    expect(searchInput).toBeVisible()
    expect(searchInput).toHaveAttribute('placeholder', 'Search')
    expect(searchInput).toHaveValue('test')
    // This is to ensure that the search icon does not get
    // pushed out of view to the left.
    expect(searchInput).toHaveStyle({ minWidth: '0px' })

    // Test search icon
    const searchIcon = container.querySelector('leo-icon')
    expect(searchIcon).toBeInTheDocument()
    expect(searchIcon).toBeVisible()
    expect(searchIcon).toHaveAttribute('name', 'search')
  })
})
