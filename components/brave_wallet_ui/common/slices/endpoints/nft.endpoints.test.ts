// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { renderHook, waitFor } from '@testing-library/react'
import {
  createMockStore,
  renderHookOptionsWithMockStore
} from '../../../utils/test-utils'
import { useGetNftAssetIdsByCollectionRegistryQuery } from '../api.slice'
import { mockMoonCatNFT } from '../../../stories/mock-data/mock-asset-options'
import { getAssetIdKey } from '../../../utils/asset-utils'

describe('NFT Endpoints', () => {
  describe('getNftAssetIdsByCollectionRegistry', () => {
    it('should group nft ids by collection name', async () => {
      const store = createMockStore({})

      const { result } = renderHook(
        () => useGetNftAssetIdsByCollectionRegistryQuery([mockMoonCatNFT]),
        renderHookOptionsWithMockStore(store)
      )

      // loading
      await waitFor(() =>
        expect(result.current.data && !result.current.isLoading).toBeTruthy()
      )

      const { data: registryInfo, isLoading, error } = result.current
      expect(isLoading).toBeFalsy()
      expect(error).toBeUndefined()
      expect(registryInfo?.registry).toEqual({
        MoonCatsRescue: [getAssetIdKey({ ...mockMoonCatNFT, tokenId: '' })]
      })
    })
  })
})
