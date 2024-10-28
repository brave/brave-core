/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

export const placeholderImageSrc =
  'data:image/svg+xml,%3Csvg xmlns="http://www.w3.org/2000/svg"%3E%3C/svg%3E'

// Loads an image in the background and resolves when the image has either
// loaded or was unable to load.
export function loadImage(url: string): Promise<boolean> {
  return new Promise((resolve) => {
    if (!url) {
      resolve(false)
      return
    }

    const unlisten = () => {
      image.removeEventListener('load', onLoad)
      image.removeEventListener('error', onError)
    }

    const onLoad = () => {
      unlisten()
      resolve(true)
    }

    const onError = () => {
      unlisten()
      resolve(false)
    }

    const image = new Image()
    image.addEventListener('load', onLoad)
    image.addEventListener('error', onError)
    image.src = url
  })
}
