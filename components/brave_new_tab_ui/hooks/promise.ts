// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import { useEffect, useState } from 'react'

export function useResult<T> (getPromise: () => Promise<T>, deps: any[]) {
  const [result, setResult] = useState<T | undefined>(undefined)
  const [loading, setLoading] = useState(true)
  const [error, setError] = useState<any>(undefined)

  useEffect(() => {
    let cancelled = false
    setLoading(true)

    getPromise().then(result => {
      if (cancelled) return

      setResult(result)
      setLoading(false)
    }).catch(err => {
      if (cancelled) return

      setResult(undefined)
      setLoading(false)
      setError(err)
    })

    return () => {
      cancelled = true
    }
  }, ...deps)

  return {
    result,
    loading,
    error
  }
}
