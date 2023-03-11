// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import { renderHook } from '@testing-library/react-hooks'

import {
  mockAssetPrices
} from '../constants/mocks'
import usePricing from './pricing'

describe('usePricing hook', () => {
    it('should return asset price of DOG token', () => {
      const { result } = renderHook(() => usePricing(mockAssetPrices))
      expect(result.current.findAssetPrice('DOG', '0xdog', '0x1')).toEqual('100')
    })

    it('should return empty asset price of unknown token', () => {
      const { result } = renderHook(() => usePricing(mockAssetPrices))
      expect(result.current.findAssetPrice('CAT', '0xcat', '0x1')).toEqual('')
    })

    it('should compute fiat amount for DOG token', () => {
      const { result } = renderHook(() => usePricing(mockAssetPrices))
      expect(result.current.computeFiatAmount('7', 'DOG', 1, '0xdog', '0x1').formatAsFiat()).toEqual('70.00')
    })

    it('should return empty fiat value for unknown token', () => {
      const { result } = renderHook(() => usePricing(mockAssetPrices))
      expect(result.current.computeFiatAmount('7', 'CAT', 0, '0xcat', '0x1').formatAsFiat()).toEqual('')
    })

    it('should return empty fiat value for empty amount', () => {
      const { result } = renderHook(() => usePricing(mockAssetPrices))
      expect(result.current.computeFiatAmount('', 'DOG', 0, '0xdog', '0x1').formatAsFiat()).toEqual('')
    })
})
