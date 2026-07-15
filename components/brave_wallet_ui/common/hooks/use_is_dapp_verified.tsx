// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Types
import { BraveWallet } from '../../constants/types'

// Hooks
import { useGetTopDappsQuery } from '../slices/api.slice'

const getUrlOrigin = (url: string): string | undefined => {
  try {
    return new URL(url).origin
  } catch {
    return undefined
  }
}

export const useIsDAppVerified = (originInfo: BraveWallet.OriginInfo) => {
  // Queries
  const { data: topDapps } = useGetTopDappsQuery()

  const requestOrigin = getUrlOrigin(originInfo.originSpec)
  const foundDApp =
    requestOrigin === undefined
      ? undefined
      : topDapps?.find((dapp) => getUrlOrigin(dapp.website) === requestOrigin)

  return { isDAppVerified: foundDApp !== undefined, dapp: foundDApp }
}
export default useIsDAppVerified
