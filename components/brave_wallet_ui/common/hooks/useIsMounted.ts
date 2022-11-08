// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

export function useIsMounted () {
  const isMounted = React.useRef(true)
  React.useEffect(() => {
    // cleanup mechanism for useEffect
    // will be called during component unmount
    return () => {
      isMounted.current = false
    }
  }, [])

  return isMounted.current
}

export default useIsMounted
