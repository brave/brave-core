// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render } from '@testing-library/react'

// components
import { NftIcon } from './nft-icon'

// mocks
import {
  createMockStore,
  renderComponentOptionsWithMockStore
} from '../../../utils/test-utils'

describe('NFT Icon', () => {
  it('Should attempt IPFS gateway lookup when given an IPFS token image URL', () => {
    const mockStore = createMockStore({})
    const dispatchSpy = jest.spyOn(mockStore, 'dispatch')

    render(
      <NftIcon icon='ipfs://QmXyZ' />,
      renderComponentOptionsWithMockStore(mockStore)
    )

    // should call the gateway lookup action
    expect(dispatchSpy).toBeCalledTimes(1)
  })

  it('Should NOT attempt IPFS gateway lookup when given an IPFS token image URL', () => {
    const mockStore = createMockStore({})
    const dispatchSpy = jest.spyOn(mockStore, 'dispatch')

    render(
      <NftIcon icon='brave.com' />,
      renderComponentOptionsWithMockStore(mockStore)
    )

    // should not call the gateway lookup action
    expect(dispatchSpy).toBeCalledTimes(0)
  })
})
