// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useSelector } from 'react-redux'

import { WalletState } from '../../constants/types'
import { makeNetworkAsset } from '../../options/asset-options'

export const useSelectedNetworkNativeAsset = () => {
  const { selectedNetwork } = useSelector(({ wallet }: { wallet: WalletState }) => wallet)

  const selectedNetworkNativeAsset = React.useMemo(
    () => makeNetworkAsset(selectedNetwork),
    [selectedNetwork]
  )

  return selectedNetworkNativeAsset
}
