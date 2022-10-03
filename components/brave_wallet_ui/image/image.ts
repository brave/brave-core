// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.
const urlParam = new URLSearchParams(window.location.search).get('targetUrl')
const imgUrl = urlParam ? urlParam.replace('chrome://image?', '') || '' : ''

const imageElement = document.getElementById('untrusted-image') as HTMLImageElement | null

if (
  imageElement &&
  imgUrl &&
  imgUrl.startsWith('https://')
) {
  imageElement.src = imgUrl
}
