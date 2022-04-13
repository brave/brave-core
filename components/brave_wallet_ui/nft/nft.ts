// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

const params = new Proxy(new URLSearchParams(window.location.search), {
  get: (searchParams, prop) => {
    if (typeof prop === 'string') {
      return searchParams.get(prop)
    }
    return null
  }
})
/* eslint-disable  @typescript-eslint/dot-notation */
const imageUrl = params['imageUrl']
const imageWidth = params['imageWidth']
const imageHeight = params['imageHeight']
console.log({ imageWidth, imageHeight })
/* eslint-enable  @typescript-eslint/dot-notation */
const imageEl = <HTMLImageElement>document.getElementById('image')

if (imageEl) {
  if (imageUrl) {
    imageEl.src = imageUrl
  }

  if (imageWidth) {
    imageEl.style.width = imageWidth
  }

  if (imageHeight) {
    imageEl.style.height = imageHeight
  }
}
