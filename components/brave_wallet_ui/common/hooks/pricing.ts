import * as React from 'react'

import { AssetPriceInfo } from '../../constants/types'

export default function usePricing (spotPrices: AssetPriceInfo[]) {
  return React.useCallback((symbol: string) => {
    return spotPrices.find(
      (token) => token.fromAsset.toLowerCase() === symbol.toLowerCase()
    )?.price ?? '0'
  }, [spotPrices])
}
