// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render } from '@testing-library/react'

// components
import { withPlaceholderIcon } from './'

// mocks
import {
  createMockStore,
  renderComponentOptionsWithMockStore
} from '../../../utils/test-utils'
import { mockErc721Token } from '../../../stories/mock-data/mock-asset-options'

const WithPlaceHolderIcon = withPlaceholderIcon(() => <div />, {
  size: 'big',
  marginLeft: 10,
  marginRight: 10
})

describe('withPlaceholderIcon', () => {
  it('Should attempt IPFS gateway lookup when given an IPFS token image URL', () => {
    const mockStore = createMockStore({})
    const dispatchSpy = jest.spyOn(mockStore, 'dispatch')

    render(
      <WithPlaceHolderIcon
        asset={{ ...mockErc721Token, logo: 'ipfs://QmXyZ' }}
      />,
      renderComponentOptionsWithMockStore(mockStore)
    )

    // should call the gateway lookup action
    expect(dispatchSpy).toBeCalledTimes(1)
  })

  it('Should NOT attempt IPFS gateway lookup when given an IPFS token image URL', () => {
    const mockStore = createMockStore({})
    const dispatchSpy = jest.spyOn(mockStore, 'dispatch')

    render(
      <WithPlaceHolderIcon asset={{ ...mockErc721Token, logo: 'brave.com' }} />,
      renderComponentOptionsWithMockStore(mockStore)
    )

    // should not call the gateway lookup action
    expect(dispatchSpy).toBeCalledTimes(0)
  })
})
