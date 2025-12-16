// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// images
import Placeholder from '../assets/svg-icons/nft-placeholder.svg'

// utils
import { DISPLAY_MODES, DisplayMode } from './nft-ui-messages'
import { NFTMetadataReturnType } from '../constants/types'

const urlParams = new URLSearchParams(window.location.search)

const displayModeParam = urlParams.get('displayMode') as DisplayMode

const displayMode =
  DISPLAY_MODES.find((mode) => mode === displayModeParam) || 'icon'

const nftMetadataParam = urlParams.get('nftMetadata')
const nftMetadata = nftMetadataParam
  ? (JSON.parse(nftMetadataParam) as NFTMetadataReturnType)
  : undefined

const mediaUrl =
  nftMetadata?.animationURL || nftMetadata?.imageURL || Placeholder

// Get the raw image URL from the icon parameter
const iconImageUrl = urlParams.get('icon') || undefined

function render() {
  const imgEl = document.getElementById('nft-image') as HTMLImageElement

  // Show placeholder if the image fails to load (e.g., due to size limits)
  imgEl.onerror = () => {
    imgEl.src = Placeholder
  }

  // Determine the image source based on display mode
  let imageSrc: string
  if (displayMode === 'details' && nftMetadata) {
    imageSrc =
      `chrome-untrusted://image?url=`
      + `${encodeURIComponent(mediaUrl)}&staticEncode=true`
  } else if (iconImageUrl) {
    imageSrc =
      `chrome-untrusted://image?url=`
      + `${encodeURIComponent(iconImageUrl)}&staticEncode=true`
  } else {
    imageSrc = Placeholder
  }

  imgEl.src = imageSrc
}

document.addEventListener('DOMContentLoaded', render)
