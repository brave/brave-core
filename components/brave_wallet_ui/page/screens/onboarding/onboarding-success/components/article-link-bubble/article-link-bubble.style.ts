// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

// icons
import KeyIcon from '../../../../../../assets/svg-icons/key-icon.svg'

export type ArticleLinkIcons =
  | 'crypto-wallets'
  | 'grid'
  | 'ipfs'
  | 'key'
  | 'smartphone-desktop'
  | 'wallet-with-coins'
  | 'wallet'

export type IconBubbleColors =
  | 'blue300'
  | 'green600'
  | 'magenta400'
  | 'orange300'
  | 'purple400'
  | 'red200'
  | 'yellow500'

const getIcon = (iconName: ArticleLinkIcons) => {
  return ({
    'wallet': '',
    'wallet-with-coins': '',
    'smartphone-desktop': '',
    'crypto-wallets': '',
    'key': KeyIcon,
    'ipfs': '',
    'grid': ''
  } as Record<ArticleLinkIcons, string>)[iconName] || ''
}

export const BubbleIconBackground = styled.div<{
  backgroundColor: IconBubbleColors
}>`
  background-color: ${(p) => p.theme.palette[p.backgroundColor]};
  border-radius: 100%;
  width: 30px;
  height: 30px;
`

export const BubbleIcon = styled.div<{
  icon: ArticleLinkIcons
}>`
  display: inline-block;
  width: 30px;
  height: 30px;

  background-color: ${(p) => p.theme.palette.white};
  mask-image: url(${p => getIcon(p.icon)});
  mask-size: 14px;
  mask-position: center center;
  mask-repeat: no-repeat;
`

export const Bubble = styled.a`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: flex-start;
  padding: 5px 12px;
  gap: 4px;
  height: 40px;

  margin-bottom: 24px;

  background: ${(p) => p.theme.palette.white};
  box-shadow: 0px 2px 8px rgba(104, 105, 120, 0.16);
  border-radius: 8px;
`

export const BubbleText = styled.span`
  height: 20px;

  font-family: 'Poppins';
  font-style: normal;
  font-weight: 600;
  font-size: 13px;
  line-height: 20px;
  display: flex;
  align-items: center;
  text-align: center;

  margin-left: 8px;

  color: ${(p) => p.theme.color.text01};
`
