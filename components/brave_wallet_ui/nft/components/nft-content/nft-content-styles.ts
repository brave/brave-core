// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import { LoaderIcon } from 'brave-ui/components/icons'
import { nftMediaSize } from '../nft-details/nft-details-styles'

export const ImageWrapper = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  width: 100%;
  height: 100%;
`

export const MultimediaWrapper = styled.div`
  position: relative;
  display: flex;
  align-items: flex-start;
  margin-right: 28px;
  flex: 1 0 ${nftMediaSize};
  height: ${nftMediaSize};
  overflow: hidden;
`

export const Image = styled.img<{
  customWidth?: string
  customHeight?: string
}>`
  display: block;
  border: transparent;
  border-radius: 4px;
  max-width: 100%;
  max-height: 100%;
  position: relative;
  object-fit: contain;
`

export const LoadingOverlay = styled.div<{ isLoading: boolean }>`
  display: ${(p) => p.isLoading ? 'flex' : 'none'};
  flex-direction: column;
  align-items: center;
  justify-content: center;
  width: 100%;
  height: 100%;
  position: absolute;
  z-index: 10;
  backdrop-filter: blur(5px);
`

export const LoadIcon = styled(LoaderIcon)`
  color: ${p => p.theme.color.interactive08};
  height: 70px;
  width: 70px;
  opacity: .4;
`
