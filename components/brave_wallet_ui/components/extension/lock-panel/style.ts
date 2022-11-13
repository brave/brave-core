// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'
import SecureIcon from '../../../assets/svg-icons/onboarding/secure-your-crypto.svg'
import SecureIconDark from '../../../assets/svg-icons/onboarding/secure-your-crypto-dark.svg'
import { WalletButton } from '../../shared/style'

export const StyledWrapper = styled.div<{ hideBackground?: boolean }>`
  display: flex;
  height: 100%;
  width: 320px;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  background-color: ${(p) => p.hideBackground ? 'transparent' : p.theme.color.background01};
`

export const Title = styled.span`
  font-family: Poppins;
  font-size: 15px;
  font-weight: 600;
  line-height: 20px;
  color: ${(p) => p.theme.color.text01};
  letter-spacing: 0.04em;
  margin-bottom: 12px;
  text-align: center;
`

export const PanelIcon = styled.div`
  width: 111px;
  height: 100px;
  background: url(${SecureIcon});
  background-repeat: no-repeat;
  background-size: 100%;
  margin-bottom: 10px;
  @media (prefers-color-scheme: dark) {
    background: url(${SecureIconDark});
  }
`

export const Column = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: column;
  width: 250px;
  margin-bottom: 8px;
`

export const RestoreButton = styled(WalletButton)`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: row;
  cursor: pointer;
  outline: none;
  background: none;
  border: none;
  font-family: Poppins;
  font-style: normal;
  font-weight: 500;
  font-size: 13px;
  line-height: 19px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.text03};
  margin-top: 12px;
`
