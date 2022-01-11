import { renderHook } from '@testing-library/react-hooks'

import {
  mockAssetPrices
} from '../constants/mocks'
import usePricing from './pricing'

describe('usePricing hook', () => {
    it('should return asset price of DOG token', () => {
      const { result } = renderHook(() => usePricing(mockAssetPrices))
      expect(result.current.findAssetPrice('DOG')).toEqual('100')
    })

    it('should return empty asset price of unknown token', () => {
      const { result } = renderHook(() => usePricing(mockAssetPrices))
      expect(result.current.findAssetPrice('CAT')).toEqual('')
    })

    it('should compute fiat amount for DOG token', () => {
      const { result } = renderHook(() => usePricing(mockAssetPrices))
      expect(result.current.computeFiatAmount('7', 'DOG', 1)).toEqual('70.00')
    })

    it('should return empty fiat value for unknown token', () => {
      const { result } = renderHook(() => usePricing(mockAssetPrices))
      expect(result.current.computeFiatAmount('7', 'CAT', 0)).toEqual('')
    })

    it('should return empty fiat value for empty amount', () => {
      const { result } = renderHook(() => usePricing(mockAssetPrices))
      expect(result.current.computeFiatAmount('', 'DOG', 0)).toEqual('')
    })
})
