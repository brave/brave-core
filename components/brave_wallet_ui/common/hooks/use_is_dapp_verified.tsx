// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Types
import { BraveWallet } from '../../constants/types'

// Hooks
import { useGetTopDappsQuery } from '../slices/api.slice'

export const useIsDAppVerified = (originInfo: BraveWallet.OriginInfo) => {
  // Queries
  const { data: topDapps } = useGetTopDappsQuery()

  const foundDApp = topDapps?.find((dapp) =>
    dapp.website.startsWith(originInfo.originSpec),
  )

  return { isDAppVerified: foundDApp !== undefined, dapp: foundDApp }
}
export default useIsDAppVerified
