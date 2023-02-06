// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { checkIfTokensMatch } from './asset-utils'
import { mockEthToken, mockBasicAttentionToken, mockMoonCatNFT } from '../stories/mock-data/mock-asset-options'

const ethToken = mockEthToken
const batToken = mockBasicAttentionToken
const nftTokenOne = mockMoonCatNFT
const nftTokenTwo = { ...mockMoonCatNFT, tokenId: '0x42a7' }

describe('Check if tokens match', () => {
  test('Comparing BAT to BAT, should match.', () => {
    expect(checkIfTokensMatch(batToken, batToken)).toBeTruthy()
  })

  test('Comparing BAT to ETH, should not match.', () => {
    expect(checkIfTokensMatch(batToken, ethToken)).toBeFalsy()
  })

  test('Comparing NFTs with the same tokenId, should match.', () => {
    expect(checkIfTokensMatch(nftTokenOne, nftTokenOne)).toBeTruthy()
  })

  test('Comparing NFTs with different tokenIds, should not match.', () => {
    expect(checkIfTokensMatch(nftTokenOne, nftTokenTwo)).toBeFalsy()
  })
})
