/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { placeholderImageSrc } from '../../lib/image_loader'

interface Props {
  src: string
  className?: string
  targetSize?: { width: number, height: number }
}

export function SafeImage(props: Props) {
  let { src } = props
  if (src) {
    src = 'chrome://brave-image?url=' + encodeURIComponent(src);
    if (props.targetSize) {
      let { width, height } = props.targetSize
      width = Math.round(width * window.devicePixelRatio)
      height = Math.round(height * window.devicePixelRatio)
      src += `&target_size=${width}x${height}`
    }
  }

  return (
    <img
      src={src || placeholderImageSrc}
      loading='lazy'
      className={props.className}
      onError={(event) => { event.currentTarget.src = placeholderImageSrc }}
      onLoad={(event) => { event.currentTarget?.classList.add('loaded') }}
    />
  )
}
