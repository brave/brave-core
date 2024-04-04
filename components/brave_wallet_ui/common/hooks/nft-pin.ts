// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// constants
import { BraveWallet } from '../../constants/types'

// utils
import { LOCAL_STORAGE_KEYS } from '../constants/local-storage-keys'
import {
  useGetAutopinEnabledQuery,
  useGetNftsPinningStatusQuery
} from '../slices/api.slice'

export enum OverallPinningStatus {
  PINNING_FINISHED,
  PINNING_IN_PROGRESS,
  NO_PINNED_ITEMS
}

export function useNftPin() {
  // queries
  const { data: isAutoPinEnabled } = useGetAutopinEnabledQuery()
  const { data: nftsPinningStatus } = useGetNftsPinningStatusQuery()

  // memos
  const pinnedNftsCount = React.useMemo(() => {
    if (!nftsPinningStatus) return 0
    return Object.keys(nftsPinningStatus).reduce(
      (accumulator, currentValue) => {
        const status = nftsPinningStatus[currentValue]
        if (status?.code === BraveWallet.TokenPinStatusCode.STATUS_PINNED) {
          return (accumulator += 1)
        }

        return accumulator
      },
      0
    )
  }, [nftsPinningStatus])

  const inProgressNftCount = React.useMemo(() => {
    if (!nftsPinningStatus) return 0
    return Object.keys(nftsPinningStatus).reduce(
      (accumulator, currentValue) => {
        const status = nftsPinningStatus[currentValue]
        if (
          status?.code ===
            BraveWallet.TokenPinStatusCode.STATUS_PINNING_IN_PROGRESS ||
          status?.code === BraveWallet.TokenPinStatusCode.STATUS_PINNING_PENDING
        ) {
          return (accumulator += 1)
        }

        return accumulator
      },
      0
    )
  }, [nftsPinningStatus])

  const pinningStatusSummary: BraveWallet.TokenPinStatusCode =
    React.useMemo(() => {
      if (pinnedNftsCount > 0 && inProgressNftCount === 0) {
        return OverallPinningStatus.PINNING_FINISHED
      } else if (inProgressNftCount > 0) {
        return OverallPinningStatus.PINNING_IN_PROGRESS
      }

      return OverallPinningStatus.NO_PINNED_ITEMS
    }, [pinnedNftsCount, inProgressNftCount])

  const [isIpfsBannerEnabled, setIsIpfsBannerEnabled] = React.useState<boolean>(
    localStorage.getItem(LOCAL_STORAGE_KEYS.IS_IPFS_BANNER_HIDDEN) ===
      'false' ||
      localStorage.getItem(LOCAL_STORAGE_KEYS.IS_IPFS_BANNER_HIDDEN) === null
  )

  const isIpfsBannerVisible = React.useMemo(() => {
    return (
      isIpfsBannerEnabled &&
      (pinningStatusSummary !== OverallPinningStatus.NO_PINNED_ITEMS ||
        !isAutoPinEnabled)
    )
  }, [isIpfsBannerEnabled, pinningStatusSummary, isAutoPinEnabled])

  // methods
  const onToggleShowIpfsBanner = React.useCallback(() => {
    setIsIpfsBannerEnabled((prev) => {
      window.localStorage.setItem(
        LOCAL_STORAGE_KEYS.IS_IPFS_BANNER_HIDDEN,
        !prev ? 'false' : 'true'
      )
      return !prev
    })
  }, [])

  return {
    pinnedNftsCount,
    isIpfsBannerVisible,
    pinningStatusSummary,
    onToggleShowIpfsBanner
  }
}
