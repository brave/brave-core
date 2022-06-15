// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

// assets
import BraveWalletSvg from '../../../../assets/svg-icons/onboarding/brave-wallet.svg'
import BraveWalletSvgDark from '../../../../assets/svg-icons/onboarding/brave-wallet-dark.svg'
import MMSvg from '../../../../assets/svg-icons/onboarding/import-from-metamask.svg'
import MMSvgDark from '../../../../assets/svg-icons/onboarding/import-from-metamask-dark.svg'
import LegacyWalletSvg from '../../../../assets/svg-icons/onboarding/reset-to-brave-wallet.svg'
import LegacyWalletSvgDark from '../../../../assets/svg-icons/onboarding/reset-to-brave-wallet-dark.svg'

// styles
import { WalletLink } from '../../../../components/shared/style'

export const CardButton = styled(WalletLink)`

  cursor: pointer;

  box-sizing: border-box;
  width: 376px;
  min-height: 88px;
  background: ${(p) => p.theme.color.background02};
  border: 1px solid ${(p) => p.theme.color.divider01};
  border-radius: 8px;

  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: space-between;

  padding-left: 24px;
  padding-right: 24px;
  margin-top: 16px;
  margin-bottom: 16px;

  &:hover {
    outline-style: solid;
    outline-color: ${p => p.theme.palette.blurple300};
    outline-width: 2px;
  }

  & > p {
    font-family: 'Poppins';
    font-style: normal;
    font-weight: 500;
    font-size: 14px;
    line-height: 26px;
    letter-spacing: 0.02em;
    color: ${(p) => p.theme.color.text01};
    text-align: left;
  }
`

export const LinkRow = styled.div`
  width: 100%;
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: flex-start;
  margin-top: 20px;
`

export const MetaMaskIcon = styled.div`
  width: 80px;
  height: 80px;
  background-image: url(${MMSvg});
  background-size: 100%;
  background-repeat: no-repeat;
  background-position: center;
  @media (prefers-color-scheme: dark) {
    background-image: url(${MMSvgDark});
  }
`

export const LegacyWalletIcon = styled.div`
  width: 80px;
  height: 80px;
  background-image: url(${LegacyWalletSvg});
  background-repeat: no-repeat;
  background-size: 100%;
  background-position: center;
  @media (prefers-color-scheme: dark) {
    background-image: url(${LegacyWalletSvgDark});
  }
`

export const BraveWalletIcon = styled.div`
  width: 80px;
  height: 80px;
  background-image: url(${BraveWalletSvg});
  background-repeat: no-repeat;
  background-size: 100%;
  background-position: center;
  @media (prefers-color-scheme: dark) {
    background-image: url(${BraveWalletSvgDark});
  }
`
