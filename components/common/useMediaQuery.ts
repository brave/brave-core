// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { useEffect, useState } from "react";

export default function useMediaQuery(query: string) {
  const [result, setResult] = useState(() => window.matchMedia(query).matches)

  useEffect(() => {
    const media = window.matchMedia(query)
    const handler = () => setResult(media.matches)
    media.addEventListener('change', handler)
    setResult(media.matches)

    return () => {
      media.removeEventListener('change', handler)
    }
  }, [query])

  return result
}
