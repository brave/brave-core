// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { selectCombinedTokensList } from './blockchain-token.entity'

// mocks
import {
  mockMoonCatNFT,
  mockErc20TokensList
} from '../../../stories/mock-data/mock-asset-options'

describe('blockchain token entity', () => {
  describe('combined tokens list selector', () => {
    it('memoizes the combined tokens list', () => {
      const knownTokensList = mockErc20TokensList
      const userTokensList = [
        knownTokensList[0], // known token
        mockMoonCatNFT
      ]

      const combinedListInstance = selectCombinedTokensList(
        knownTokensList,
        userTokensList
      )

      const secondCombinedListInstance = selectCombinedTokensList(
        knownTokensList,
        userTokensList
      )

      expect(combinedListInstance).not.toBe([...secondCombinedListInstance])
      expect(combinedListInstance).toBe(secondCombinedListInstance)

      // like-tokens should only be included in the list once
      expect(combinedListInstance.length).toBe(mockErc20TokensList.length + 1)
    })
  })
})
