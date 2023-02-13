// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import { WalletButton } from '../../../../../shared/style'
import Ipfs from '../../../../../../assets/svg-icons/nft-ipfs/ipfs-color.svg'
import MoreVertical from '../../../../../../assets/svg-icons/more-vertical.svg'

export const StyledWrapper = styled.div`
  display: grid;
  grid-template-columns: repeat(5, 1fr);
  grid-gap: 25px;
  box-sizing: border-box;
  width: 100%;
  padding-top: 10px;
  @media screen and (max-width: 1350px) {
    grid-template-columns: repeat(4, 1fr);
  }
  @media screen and (max-width: 1150px) {
    grid-template-columns: repeat(3, 1fr);
  }
  @media screen and (max-width: 950px) {
    grid-template-columns: repeat(2, 1fr);
  }
`

export const NFTWrapper = styled.div`
  display: flex;
  align-items: flex-start;
  justify-content: flex-start;
  position: relative;
  box-sizing: border-box;
  flex-direction: column;
  cursor: pointer;
  outline: none;
  background: none;
  border: none;
  margin: 0px;
  padding: 2px;
  overflow: hidden;
  &:last-child {
    margin-right: 0px;
  }
  z-index: 1;
`

export const IconWrapper = styled.div`
  position: relative;
  overflow: visible;
  width: 100%;
  padding-top: 100%;
  z-index: 0;
`

export const DIVForClickableArea = styled.div`
  display: block;
  position: absolute;
  z-index: 4;
  top: 0;
  left: 0;
  bottom: 0;
  right: 0;
  width: 100%;
  height: 100%;
  z-index: 2;
`

export const NFTText = styled.span`
  font-family: Poppins;
  font-size: 14px;
  line-height: 20px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.text01};
  margin-top: 6px;
  max-width: 99%;
  white-space: nowrap;
  text-overflow: ellipsis;
  overflow: hidden;
`

export const NFTSymbol = styled.span`
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 400;
  font-size: 14px;
  line-height: 20px;
  color: ${p => p.theme.color.text03};
  margin-top: 4px;
`

export const PinnedIcon = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  position: absolute;
  bottom: 12px;
  right: 12px;
  width: 20px;
  height: 20px;
  background-image: url(${Ipfs});
  background-repeat: no-repeat;
  z-index: 2;
`

export const VerticalMenu = styled(WalletButton)`
  display: flex;
  align-items: center;
  justify-content: center;
  width: 20px;
  height: 20px;
  background-color: transparent;
  position: absolute;
  right: 8px;
  top: 8px;
  z-index: 3;
  border: none;
  cursor: pointer;
`

export const VerticalMenuIcon = styled.div`
  width: 4px;
  height: 14px;
  filter: drop-shadow(0px 1px 4px rgba(0, 0, 0, 0.1));
  mask-image: url(${MoreVertical});
  -webkit-mask-image: url(${MoreVertical});
  background-color: #fff;
`
