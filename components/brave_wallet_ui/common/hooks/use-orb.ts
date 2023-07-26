// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import * as EthereumBlockies from 'ethereum-blockies'
import * as crypto from 'crypto'

import { BraveWallet } from '../../constants/types'

interface OrbOptions {
  size: number
  scale: number
}

const applyDefaults = (options: Partial<OrbOptions> | undefined): OrbOptions => {
  return { size: options?.size || 8, scale: options?.scale || 16 }
}

export const useAccountOrb = (
  accountInfo:
    | Pick<BraveWallet.AccountInfo, 'accountId' | 'address'>
    | undefined,
  options?: Partial<OrbOptions>
) => {
  return React.useMemo(() => {
    if (!accountInfo) {
      return ''
    }

    // Using hash of uniqueKey so similar unique keys don't produce similar colors.
    const seed =
      accountInfo.address?.toLowerCase() ||
      crypto
        .createHash('sha256')
        .update(accountInfo.accountId.uniqueKey)
        .digest('hex')

    return (
      EthereumBlockies.create({
        seed,
        ...applyDefaults(options)
      }) as HTMLCanvasElement
    ).toDataURL()
  }, [accountInfo?.address, accountInfo?.accountId.uniqueKey, options?.size, options?.scale])
}

export const useAddressOrb = (
  address: string | undefined,
  options?: Partial<OrbOptions>
) => {
  return React.useMemo(() => {
    if (!address) {
      return ''
    }

    return (
      EthereumBlockies.create({
        seed: address.toLowerCase(),
        ...applyDefaults(options)
      }) as HTMLCanvasElement
    ).toDataURL()
  }, [address, options?.size, options?.scale])
}

export const useNetworkOrb = (
  networkInfo: Pick<BraveWallet.NetworkInfo, 'chainName'> | undefined,
  options?: Partial<OrbOptions>
) => {
  return React.useMemo(() => {
    if (!networkInfo) {
      return ''
    }

    return (
      EthereumBlockies.create({
        seed: networkInfo.chainName,
        ...applyDefaults(options)
      }) as HTMLCanvasElement
    ).toDataURL()
  }, [networkInfo, options?.size, options?.scale])
}
