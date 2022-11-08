// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.s

import styled from 'styled-components'
import CheckMark from '../../../assets/svg-icons/big-checkmark.svg'
import SwitchDown from '../../../assets/svg-icons/switch-icon.svg'
import { WalletButton } from '../../shared/style'

export const StyledWrapper = styled.div<{ panelBackground: string }>`
  display: flex;
  height: 100%;
  width: 100%;
  flex-direction: column;
  align-items: center;
  justify-content: space-between;
  background: ${(p) => p.panelBackground};
`

export const CenterColumn = styled.div`
  display: flex;
  width: 100%;
  flex: 1;
  flex-direction: column;
  align-items: center;
  justify-content: space-between;
  padding: 12px 0px 20px;
  max-width: 300px;
`

export const AccountCircle = styled(WalletButton) <{ orb: string }>`
  display: flex;
  cursor: pointer;
  width: 54px;
  height: 54px;
  border-radius: 100%;
  background-image: url(${(p) => p.orb});
  background-size: cover;
  border: 2px solid white;
  position: relative;
  box-sizing: border-box;
  margin-bottom: 6px;
`

export const AccountNameText = styled.span`
  font-family: Poppins;
  font-size: 13px;
  line-height: 20px;
  font-weight: 600;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.palette.white};
`

export const AccountAddressText = styled(WalletButton)`
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.palette.white};
  font-weight: 300;
  cursor: pointer;
  outline: none;
  background: none;
  border: none;
`

export const BalanceColumn = styled.div`
  display: flex;
  width: 100%;
  flex-direction: column;
  align-items: center;
  justify-content: center;
`

export const AssetBalanceText = styled.span`
  font-family: Poppins;
  font-size: 24px;
  line-height: 36px;
  letter-spacing: 0.02em;
  color: ${(p) => p.theme.palette.white};
  font-weight: 600;
  height: 36px;
`

export const FiatBalanceText = styled.span`
  font-family: Poppins;
  font-size: 13px;
  line-height: 20px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.palette.white};
  font-weight: 300;
  height: 20px;
`

export const NotConnectedIcon = styled.div`
  width: 14px;
  height: 14px;
  margin-right: 8px;
  border-radius: 24px;
  border: 1px solid rgba(255,255,255,0.5);
`

export const OvalButton = styled(WalletButton)`
  display: flex;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  outline: none;
  background: none;
  border-radius: 48px;
  padding: 3px 10px;
  border: 1px solid rgba(255,255,255,0.5);
  color: ${(p) => p.theme.palette.white};
  &:disabled {
    cursor: default;
  }
`

export const OvalButtonText = styled.span`
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.palette.white};
  font-weight: 600;
`

export const StatusRow = styled.div`
  display: flex;
  width: 100%;
  flex-direction: row;
  align-items: center;
  justify-content: space-between;
  padding: 0px 12px;
`

export const BigCheckMark = styled.div`
  width: 14px;
  height: 14px;
  background-color: ${(p) => p.theme.palette.white};
  -webkit-mask-image: url(${CheckMark});
  mask-image: url(${CheckMark});
  margin-right: 8px;
`

export const SwitchIcon = styled.div`
  width: 14px;
  height: 14px;
  background: url(${SwitchDown});
  position: absolute;
  left: 0px;
  bottom: 0px;
  z-index: 10;
`

export const MoreAssetsButton = styled(WalletButton)`
  cursor: pointer;
  outline: none;
  background: none;
  border: none;
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.palette.white};
`

export const ConnectedStatusBubble = styled.div<{ isConnected: boolean }>`
  display: flex;
  align-items: center;
  justify-content: center;
  width: 10px;
  height: 10px;
  border-radius: 100%;
  background-color: ${(p) => p.isConnected ? p.theme.color.successBorder : p.theme.color.errorBorder};
  margin-right: 6px;
  border: 1px solid rgba(255,255,255, 1);
`
