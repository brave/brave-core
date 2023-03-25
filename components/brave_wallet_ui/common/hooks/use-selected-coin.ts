// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// hooks
import { useGetSelectedChainQuery } from '../slices/api.slice'

export const useSelectedCoinQuery = (
  opts?: { skip?: boolean }
) => {
  const queryResults = useGetSelectedChainQuery(undefined, {
    selectFromResult: (res) => ({ selectedCoin: res.data?.coin }),
    skip: opts?.skip
  })
  return queryResults
}
