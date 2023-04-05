// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'

import IpfsIcon from '../../../assets/svg-icons/nft-ipfs/ipfs-small.svg'
import IpfsUploadingIcon from '../../../assets/svg-icons/nft-ipfs/ipfs-uploading.svg'

export const StyledWrapper = styled.div<{ size: string }>`
  display: flex;
  align-items: center;
  justify-content: center;
  width: ${p => p.size};
  height: ${p => p.size};
`

export const GifWrapper = styled.div`
  display: flex;
  position: relative;
  align-items: center;
  justify-content: center;
  width: 30px;
  height: 30px;
`

export const StatusGif = styled.img`
  width: 100%;
  height: 100%;
  z-index: 1;
`

const BannerLeftIcon = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  width: 14px;
  height: 14px;
  background-color: ${(p) => p.theme.palette.white};
`

export const Ipfs = styled(BannerLeftIcon)<{ size?: string }>`
  -webkit-mask-image: url(${IpfsIcon});
  mask-image: url(${IpfsIcon});
  mask-repeat: no-repeat;
  width: ${p => p.size};
  height: ${p => p.size};
`

export const IpfsUploading = styled(BannerLeftIcon)`
  background-image: url(${IpfsUploadingIcon});
  background-repeat: no-repeat;
  background-size: cover;
  background-position: 0 0;
  background-color: transparent;
  position: absolute;
  width: 23px;
  height: 23px;
  z-index: 2;
`
