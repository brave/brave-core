// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'
import { WalletButton } from '../../shared/style'

import CloseIcon from '../../../assets/svg-icons/close.svg'
import BannerBackground from '../../../assets/svg-icons/nft-ipfs/banner-background.svg'

import { BannerStatus } from './nft-ipfs-banner'

const getSuccessBackground = () => {
  return window.matchMedia('(prefers-color-scheme: dark)').matches
    ? '#CBF1D2'
    : '#EEFBF0'
}

export const StyledWrapper = styled.div<{ status: BannerStatus }>`
  display: flex;
  flex-direction: row;
  align-items: center;
  width: 100%;
  justify-content: flex-start;
  background: ${(p) =>
    p.status === 'start'
      ? `url(${BannerBackground}) right 80px center/contain no-repeat, linear-gradient(110.74deg, #242464 -10.97%, #000027 173.98%)`
      : p.status === 'uploading'
      ? '#F0F7FC'
      : getSuccessBackground()};
  border-radius: 8px;
  padding: 10px 25px;
  border-width: 0;
`

export const Text = styled.p<{ status: BannerStatus }>`
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 300;
  font-size: 14px;
  line-height: 20px;
  align-items: center;
  color: ${(p) => (p.status === 'start' ? p.theme.palette.white : '#1D1F25')};
  padding: 0;
  margin: 0;
  max-width: 70%;

  @media (min-width: 1920px) {
    max-width: 65%;
  }
`

export const LearnMore = styled(WalletButton)`
  display: inline-flex;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 500;
  font-size: 14px;
  line-height: 20px;
  color: ${(p) => p.theme.palette.white};
  outline: none;
  border: none;
  background: transparent;
`

export const CloseButton = styled(WalletButton)<{ status: BannerStatus }>`
  display: flex;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  width: 20px;
  height: 20px;
  background-color: ${(p) =>
    p.status === 'success' ? '#6B7084' : p.theme.palette.white};
  -webkit-mask-image: url(${CloseIcon});
  mask-image: url(${CloseIcon});
  outline: none;
  border: none;
  margin-left: auto;
`
