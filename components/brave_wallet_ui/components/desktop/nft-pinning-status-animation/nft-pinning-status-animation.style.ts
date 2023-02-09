// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'

import IpfsIcon from '../../../assets/svg-icons/nft-ipfs/ipfs.svg'
import IpfsSuccessIcon from '../../../assets/svg-icons/nft-ipfs/ipfs-success.svg'

export const StyledWrapper = styled.div<{ size: string }>`
  display: flex;
  align-items: center;
  justify-content: center;
  width: ${p => p.size};
  height: ${p => p.size};
`

export const GifWrapper = styled.div<{ size?: string}>`
  display: flex;
  position: relative;
  align-items: center;
  justify-content: center;
  width: ${p => p.size};
  height: ${p => p.size};
`

export const StatusGif = styled.img`
  width: 100%;
  height: 100%;
  z-index: 0;
  mix-blend-mode: multiply; /* make gif background blend with parent background */
`

const BannerLeftIcon = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  width: 24px;
  height: 24px;
  background-color: ${(p) => p.theme.palette.white};
`

export const Ipfs = styled(BannerLeftIcon)<{ size?: string}>`
  -webkit-mask-image: url(${IpfsIcon});
  mask-image: url(${IpfsIcon});
  mask-repeat: no-repeat;
  width: ${p => p.size};
  height: ${p => p.size};
`

export const IpfsUploading = styled(BannerLeftIcon)`
  background-image: url(${IpfsSuccessIcon});
  background-repeat: no-repeat;
  background-size: cover;
  background-position: 0 0;
  background-color: transparent;
  z-index: 0;
  position: absolute;
`
