// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { renderHook, waitFor } from '@testing-library/react'

import {
  createMockStore,
  renderHookOptionsWithMockStore,
} from '../../utils/test-utils'

import useGetTokenInfo from './use-get-token-info'

// mocks
import { mockNetwork } from '../constants/mocks'
import {
  mockAccountAssetOptions,
  mockErc20TokensList,
} from '../../stories/mock-data/mock-asset-options'

describe('useGetTokenInfo hook', () => {
  it('Should return token info from tokens list', async () => {
    const store = createMockStore(
      {},
      {
        blockchainTokens: mockErc20TokensList,
        userAssets: mockAccountAssetOptions,
      },
    )
    const renderOptions = renderHookOptionsWithMockStore(store)

    const { result } = renderHook(
      () =>
        useGetTokenInfo({
          contractAddress: mockErc20TokensList[0].contractAddress,
          network: mockNetwork,
        }),
      renderOptions,
    )

    // initial state
    expect(result.current.tokenInfo).toBeUndefined()
    expect(result.current.isLoading).toBeDefined()
    expect(result.current.isLoading).toBe(true)

    // loading
    await waitFor(() =>
      expect(
        result.current.tokenInfo !== undefined && !result.current.isLoading,
      ).toBeTruthy(),
    )

    // done loading
    expect(result.current.isLoading).toBe(false)
    expect(result.current.tokenInfo?.name).toEqual(mockErc20TokensList[0].name)
  })

  it('Should return token info for unknown contract from RPC', async () => {
    const store = createMockStore(
      {},
      {
        blockchainTokens: mockErc20TokensList,
        userAssets: mockAccountAssetOptions,
      },
    )
    const renderOptions = renderHookOptionsWithMockStore(store)

    const { result } = renderHook(
      () =>
        useGetTokenInfo({
          contractAddress: '0xdeadbeef',
          network: mockNetwork,
        }),
      renderOptions,
    )

    // initial state
    expect(result.current.tokenInfo).toBeUndefined()
    expect(result.current.isLoading).toBeDefined()
    expect(result.current.isLoading).toBe(true)

    // loading
    await waitFor(() =>
      expect(
        result.current.tokenInfo !== undefined && !result.current.isLoading,
      ).toBeTruthy(),
    )

    // done loading
    expect(result.current.isLoading).toBe(false)
    expect(result.current.tokenInfo?.name).toEqual('Mocked Token')
  })

  // Follow up issue to fix test via https://github.com/brave/brave-browser/issues/43583

  // it('Should return an error if token info is not found', async () => {
  //   const store = createMockStore(
  //     {},
  //     {
  //       blockchainTokens: mockErc20TokensList,
  //       userAssets: mockAccountAssetOptions
  //     }
  //   )

  //   const renderOptions = renderHookOptionsWithMockStore(store)

  //   const { result } = renderHook(
  //     () =>
  //       useGetTokenInfo({
  //         contractAddress: '0xInvalidToken',
  //         network: mockNetwork
  //       }),
  //     renderOptions
  //   )

  //   // initial state
  //   expect(result.current.tokenInfo).toBeUndefined()
  //   expect(result.current.isLoading).toBeDefined()

  //   // loading
  //   await waitFor(() => expect(result.current.isLoading).toBeFalsy())

  //   // done loading
  //   expect(result.current.isLoading).toBe(false)
  //   expect(result.current.tokenInfo).toBe(undefined)
  //   expect(result.current.isError).toBe(true)
  // })
})
