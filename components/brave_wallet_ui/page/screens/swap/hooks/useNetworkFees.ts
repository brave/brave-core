// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import React from 'react'

// Hooks
import { useWalletState } from '~/state/wallet'
import { useSwapContext } from '~/context/swap.context'

// Utils
import Amount from '~/utils/amount'

// Types
import { NetworkInfo } from '~/constants/types'

export const useNetworkFees = () => {
  const { defaultBaseCurrency } = useSwapContext()

  // Wallet State
  const {
    state: { spotPrices, networkFeeEstimates }
  } = useWalletState()

  const getNetworkFeeFiatEstimate = React.useCallback(
    (network: NetworkInfo) => {
      if (!networkFeeEstimates[network.chainId]) {
        return ''
      }
      if (spotPrices.nativeAsset === '') {
        return ''
      }

      // FIXME - this should be coming from quotes
      // networkFeeEstimates should be removed or kept purely
      // for presentational purpose
      return new Amount(spotPrices.nativeAsset)
        .times(networkFeeEstimates[network.chainId].gasFee)
        .formatAsFiat(defaultBaseCurrency)
    },
    [spotPrices, networkFeeEstimates, defaultBaseCurrency]
  )

  return {
    getNetworkFeeFiatEstimate
  }
}
export default useNetworkFees
