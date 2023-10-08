// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useLocation } from 'react-router-dom'

/**
 * This hook is used to get query params from the current route.
 * It is a wrapper around the `URLSearchParams` API.
 *
 * Example usage:
 *
 * const query = useQuery()
 * const param = query.get('param')
 * @returns {URLSearchParams} The query params from the current route.
 */
export function useQuery() {
  const { search } = useLocation()

  return React.useMemo(() => new URLSearchParams(search), [search])
}
