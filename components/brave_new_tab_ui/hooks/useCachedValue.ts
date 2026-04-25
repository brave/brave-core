// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import { useCallback, useEffect, useState } from 'react'

export const useCachedValue = <T>(value: T, setValue: (newValue: T) => void) => {
  const [cached, setCached] = useState<T>(value)
  const updateCached = useCallback((newValue: T) => {
    setCached(newValue)
    setValue(newValue)
  }, [setValue])

  useEffect(() => {
    setCached(value)
  }, [value])

  return [cached, updateCached] as const
}
