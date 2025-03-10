/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { useAppActions } from '../context/app_model_context'
import { placeholderImageSrc } from '../../lib/image_loader'

interface Props {
  src: string
  className?: string
}

export function PcdnImage(props: Props) {
  const actions = useAppActions()
  const [imageURL, setImageURL] = React.useState(placeholderImageSrc)

  React.useEffect(() => {
    setImageURL(placeholderImageSrc)
    actions.getPcdnImageURL(props.src).then(setImageURL)
  }, [props.src])

  return <img src={imageURL} className={props.className} />
}
