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
    ? '#15371B' // Leo Value: Dark/SystemFeedback/Success-Background
    : '#E8F9EB' // Leo Value: Light/SystemFeedback/Success-Background
}

const getUploadingBackground = () => {
  return window.matchMedia('(prefers-color-scheme: dark)').matches
    ? '#063256' // Leo Value: Dark/SystemFeedback/Info-Background
    : '#F0F7FC' // Leo Value: Light/SystemFeedback/Info-Background
}

export const StyledWrapper = styled.div<{ status: BannerStatus }>`
  display: flex;
  flex-direction: row;
  align-items: center;
  width: 100%;
  justify-content: flex-start;
  background: ${(p) =>
    p.status === 'start'
      ? `url(${BannerBackground}) right 80px center/contain no-repeat, #1A1C3B`
      : p.status === 'uploading'
      ? getUploadingBackground()
      : getSuccessBackground()};
  border-radius: 8px;
  border-width: 0;
  height: 67px;
  padding-left: 18px;
  padding-right: 18px;
`

export const Text = styled.p<{ status: BannerStatus }>`
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 300;
  font-size: 14px;
  line-height: 20px;
  align-items: center;
  color: ${(p) => (p.status === 'start' ? p.theme.palette.white : window.matchMedia('(prefers-color-scheme: dark)').matches ? '#ECEFF2' : '#1D1F25')};
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
  font-weight: 600;
  font-size: 14px;
  line-height: 20px;
  color: #AAA8F7; /* Leo theme value: Dark/Text/Interactive */
  outline: none;
  border: none;
  background: transparent;
  margin-left: auto;
`

export const CloseButton = styled(WalletButton)`
  display: flex;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  width: 20px;
  height: 20px;
  background-color: #6B7084;
  @media (prefers-color-scheme: dark) {
    background-color: #8C90A1; /* Leo theme value: Icon/Default */
  }
  -webkit-mask-image: url(${CloseIcon});
  mask-image: url(${CloseIcon});
  outline: none;
  border: none;
  margin-left: auto;
`
