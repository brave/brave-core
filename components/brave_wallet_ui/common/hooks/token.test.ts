// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import { renderHook, act } from '@testing-library/react-hooks'
import { GetBlockchainTokenInfoReturnInfo } from '../../constants/types'
import useTokenInfo from './token'

// mocks
import { mockNetwork } from '../constants/mocks'
import { mockERC20Token } from '../../stories/mock-data/mock-asset-options'

const findBlockchainTokenInfo = async (address: string) => {
  return { token: null } as GetBlockchainTokenInfoReturnInfo
}

describe('useTokenInfo hook', () => {
  it('Should return DOG token info from tokens list', () => {
    const { result } = renderHook(() =>
      useTokenInfo(findBlockchainTokenInfo, [mockERC20Token], mockNetwork)
    )
    act(() =>
      result.current.onFindTokenInfoByContractAddress('mockContractAddress')
    )
    expect(result.current.foundTokenInfoByContractAddress?.name).toEqual(
      'Dog Coin'
    )
  })

  it('Should not find info and return undefined', () => {
    const { result } = renderHook(() =>
      useTokenInfo(findBlockchainTokenInfo, [mockERC20Token], mockNetwork)
    )
    act(() =>
      result.current.onFindTokenInfoByContractAddress('testContractAddress')
    )
    expect(result.current.foundTokenInfoByContractAddress?.name).toEqual(
      undefined
    )
  })
})
