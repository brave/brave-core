// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import { WalletButton } from '../../../../../shared/style'
import * as leo from '@brave/leo/tokens/css'
import Icon from '@brave/leo/react/icon'
import Ipfs from '../../../../../../assets/svg-icons/nft-ipfs/ipfs-color.svg'

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
  padding-right: 2px;
  overflow: hidden;
  &:last-child {
    margin-right: 0px;
  }
  z-index: 0;
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
  font-style: normal;
  font-weight: 600;
  line-height: 24px;
  color: ${leo.color.text.primary};
  margin-top: 6px;
  max-width: 99%;
  white-space: nowrap;
  text-overflow: ellipsis;
  overflow: hidden;
`

export const NFTSymbol = styled.span`
  font-family: Poppins;
  font-size: 12px;
  font-style: normal;
  font-weight: 400;
  line-height: 18px;
  color: ${leo.color.text.secondary};
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
  width: 28px;
  height: 28px;
  background-color: transparent;
  position: absolute;
  right: 8px;
  top: 8px;
  z-index: 3;
  border: none;
  cursor: pointer;
  background: rgba(102, 109, 137, 0.5);
  backdrop-filter: blur(27.5px);
  border-radius: 1000px;
`

export const MoreButton = styled(WalletButton)`
  background-color: transparent;
  border: none;
  outline: none;
  cursor: pointer;
  z-index: 3;
`

export const MoreIcon = styled(Icon).attrs({
  name: 'more-horizontal'
})`
  --leo-icon-size: 22px;
  color: ${leo.color.text.secondary};
`

export const JunkMarker = styled.div`
  display: inline-flex;
  height: 20px;
  padding: 0px ${leo.spacing.s};
  align-items: center;
  gap: ${leo.spacing.s};
  position: absolute;
  top: 12px;
  left: 12px;
  flex-shrink: 0;
  border-radius: ${leo.radius.s};
  background-color: ${leo.color.red[20]};
  color: ${leo.color.red[50]};
  font-family: Poppins;
  font-size: 10px;
  font-style: normal;
  font-weight: 600;
  line-height: normal;
  text-transform: uppercase;
  z-index: 2;
`

export const JunkIcon = styled(Icon).attrs({
  name: 'warning-triangle-outline'
})`
  --leo-icon-size: 14px;
  color: ${leo.color.systemfeedback.errorIcon};
`
