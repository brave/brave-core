// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// images
import Placeholder from '../assets/svg-icons/nft-placeholder.svg'

// utils
import { DISPLAY_MODES, DisplayMode } from './nft-ui-messages'
import { NFTMetadataReturnType } from '../constants/types'
import { stripChromeImageURL } from '../utils/string-utils'

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

const imageUrlParam = decodeURI(urlParams.get('icon') || '') || undefined
const imageUrl = imageUrlParam
  ? imageUrlParam.endsWith('/')
    ? imageUrlParam.slice(0, imageUrlParam.length - 1)
    : imageUrlParam
  : undefined
const imageSrc = stripChromeImageURL(imageUrl) ?? Placeholder

function render() {
  const imgEl = document.getElementById('nft-image') as HTMLImageElement
  // update img src
  imgEl.src = displayMode === 'details' && nftMetadata ? mediaUrl : imageSrc
}

document.addEventListener('DOMContentLoaded', render)
