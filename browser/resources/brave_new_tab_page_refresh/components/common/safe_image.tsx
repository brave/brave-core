/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { placeholderImageSrc } from '../../lib/image_loader'

interface Props {
  src: string
  className?: string
}

export function SafeImage(props: Props) {
  return (
    <img
      src={
        props.src
          ? 'chrome://image?url=' + encodeURIComponent(props.src)
          : placeholderImageSrc
      }
      loading='lazy'
      className={props.className}
      onError={(event) => { event.currentTarget.src = placeholderImageSrc }}
    />
  )
}
