// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

// types
import { BraveWallet } from '../../constants/types'

// utils
import { stripERC20TokenImageURL } from '../../utils/string-utils'

// images & icons
import transparent40x40Image from '../../assets/png-icons/transparent40x40.png'
import EyeOnIcon from '../../assets/svg-icons/eye-on-icon.svg'
import EyeOffIcon from '../../assets/svg-icons/eye-off-icon.svg'
import CheckmarkSvg from '../../assets/svg-icons/big-checkmark.svg'
import CloseSvg from '../../assets/svg-icons/close.svg'
import { Link } from 'react-router-dom'

// Text
export const ErrorText = styled.span`
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  color: ${(p) => p.theme.color.errorText};
  margin-bottom: 10px;
`

// Containers
export const StatusBubble = styled.div<{ status: BraveWallet.TransactionStatus }>`
  display: flex;
  align-items: center;
  justify-content: center;
  width: 10px;
  height: 10px;
  border-radius: 100%;
  opacity: ${(p) => p.status === BraveWallet.TransactionStatus.Submitted ||
    p.status === BraveWallet.TransactionStatus.Approved ||
    p.status === BraveWallet.TransactionStatus.Unapproved
    ? 0.4
    : 1
  };
  background-color: ${(p) => p.status === BraveWallet.TransactionStatus.Confirmed || p.status === BraveWallet.TransactionStatus.Approved
    ? p.theme.color.successBorder
    : p.status === BraveWallet.TransactionStatus.Rejected || p.status === BraveWallet.TransactionStatus.Error || p.status === BraveWallet.TransactionStatus.Dropped ? p.theme.color.errorBorder
      : p.status === BraveWallet.TransactionStatus.Unapproved ? p.theme.color.interactive08 : p.theme.color.warningIcon
  };
  margin-right: 6px;
`

// Icons
export interface AssetIconProps {
  icon?: string
}

export const AssetIconFactory = styled.img.attrs<AssetIconProps>(props => ({
  src: stripERC20TokenImageURL(props.icon)
    ? props.icon

    // Display theme background (using a transparent image) if no icon to
    // render.
    : transparent40x40Image,

  // Defer loading the image until it reaches a calculated distance from the
  // viewport, as defined by the browser.
  //
  // Ref: https://web.dev/browser-level-image-lazy-loading
  loading: 'lazy'
}))

export const GreenCheckmark = styled.div`
  display: inline-block;
  width: 10px;
  height: 10px;
  margin-right: 4px;
  background-color: ${(p) => p.theme.color.successBorder};
  mask: url(${CheckmarkSvg}) no-repeat 50% 50%;
  mask-size: contain;
  vertical-align: middle;
`

export const ErrorXIcon = styled.div`
  width: 12px;
  height: 12px;
  background-color: ${(p) => p.theme.color.errorIcon};
  -webkit-mask-image: url(${CloseSvg});
  mask-image: url(${CloseSvg});
  mask-size: 12px;
  mask-position: center center;
  margin-right: 10px;
  margin-bottom: 10px;
  display: inline-block;
`

// Buttons
export const WalletButton = styled.button<{
  isDraggedOver?: boolean
}>`
  &:focus-visible {
    outline-style: solid;
    outline-color: ${p => p.theme.palette.blurple300};
    outline-width: 2px;
  }
 `

export const WalletLink = styled(Link)`
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 600;
  font-size: 13px;
  line-height: 20px;
  text-align: center;
  color: ${(p) => p.theme.color.interactive05};
  background: none;
  border: none;

  &:hover {
    cursor: pointer;
  }
  
  &:active {
    opacity: 0.5;
  }
`

export const WalletButtonLink = styled(Link)<{
  isDraggedOver?: boolean
}>`
  &:focus-visible {
    outline-style: solid;
    outline-color: ${p => p.theme.palette.blurple300};
    outline-width: 2px;
  }
 `

export const ToggleVisibilityButton = styled(WalletButton)<{
  isVisible: boolean
}>`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: row;
  cursor: pointer;
  outline: none;
  background: none;
  border: none;
  padding: 0px;
  width: 18px;
  height: 18px;
  background-color: ${(p) => p.theme.color.text02};
  -webkit-mask-image: url(${(p) => p.isVisible ? EyeOffIcon : EyeOnIcon});
  mask-image: url(${(p) => p.isVisible ? EyeOffIcon : EyeOnIcon});
  mask-size: contain;
  mask-position: center;
  mask-repeat: no-repeat;
`

// Spacers
export const VerticalSpacer = styled.div<{ space: number | string }>`
  display: flex;
  height: ${p => typeof p.space === 'number' ? `${p.space}px` : p.space};
`
