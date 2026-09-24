// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from "react";

export default function Image({ src, ...rest }: React.DetailedHTMLProps<React.ImgHTMLAttributes<HTMLImageElement>, HTMLImageElement>) {
  const ref = React.useRef<HTMLImageElement>(null)
  React.useEffect(() => {
    if (!ref.current) return
    ref.current.setAttribute('style', 'opacity: 0')

    const loadHandler = () => {
      ref.current?.setAttribute('style', '')
    }

    const errorHandler = () => {
      ref.current?.setAttribute('style', 'opacity: 0')
    }

    ref.current.addEventListener('load', loadHandler)
    ref.current.addEventListener('error', errorHandler)

    return () => {
      ref.current?.removeEventListener('load', loadHandler)
      ref.current?.removeEventListener('error', errorHandler)
    }
  }, [src])
  return <img ref={ref} src={src} {...rest} />
}
