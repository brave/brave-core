// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import { LoaderIcon } from 'brave-ui/components/icons'
import * as leo from '@brave/leo/tokens/css/variables'
import Checkbox from '@brave/leo/react/checkbox'

import TrezorLight from './images/trezor_light.svg'
import TrezorDark from './images/trezor_dark.svg'
import LedgerLight from './images/ledger_light.svg'
import LedgerDark from './images/ledger_dark.svg'
import { BraveWallet } from '../../../constants/types'

export const HardwareWalletGraphic = styled.div<{
  hardwareVendor: BraveWallet.HardwareVendor
}>`
  display: flex;
  flex-direction: column;
  justify-content: flex-end;
  width: 321px;
  height: 179px;

  background-image: ${(p) =>
    `url(${
      p.hardwareVendor === BraveWallet.HardwareVendor.kTrezor
        ? TrezorLight
        : LedgerLight
    })`};

  @media (prefers-color-scheme: dark) {
    background-image: ${(p) =>
      `url(${
        p.hardwareVendor === BraveWallet.HardwareVendor.kTrezor
          ? TrezorDark
          : LedgerDark
      })`};
  }
  background-color: ${leo.color.container.background};
  background-size: contain;
  background-repeat: no-repeat;
  position: relative;
`

interface StyleProps {
  isSelected: boolean
  size: 'big' | 'small'
}

interface AccountCircleStyleProps {
  orb: string
}

export const HardwareWalletAccountCircle = styled.div<AccountCircleStyleProps>`
  width: 40px;
  height: 40px;
  border-radius: 100%;
  background-image: url(${(p) => p.orb});
  background-size: cover;
`

export const HardwareWalletAccountsListContainer = styled.div`
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  padding: 0px;
  width: 100%;
  height: 300px;
  overflow-y: auto;
  margin-bottom: 15px;
  ::-webkit-scrollbar {
    //width: 0;  /* Remove scrollbar space */
    //background: transparent;  /* Optional: just make scrollbar invisible */
  }
`

export const HardwareWalletAccountListItem = styled.div`
  display: flex;
  flex-direction: row;
  margin: 16px 0px;
  width: 100%;
  padding: 0 16px;
  gap: 16px;
`

export const HardwareWalletAccountListItemRow = styled.div`
  flex: 1;
  flex-direction: row;
  font-family: Poppins;
  font-style: normal;
  font-weight: 400;
  font-size: 14px;
  line-height: 20px;
  display: flex;
  align-items: center;
  letter-spacing: 0.01em;

  color: ${leo.color.text.primary};

  justify-content: space-between;
`

export const AddressBalanceWrapper = styled.div`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: flex-start;
  height: 100%;
`

export const ButtonsContainer = styled.div`
  display: flex;
  flex-direction: row;
  justify-content: center;
  width: 100%;
  gap: 10px;

  & leo-button {
    flex-grow: 0;
  }
`

export const SelectRow = styled.div`
  display: flex;
  flex-direction: row;
  align-items: flex-start;
  justify-content: flex-start;
  width: 100%;
  margin-top: 10px;
  margin-bottom: 10px;
`

export const SelectWrapper = styled.div`
  width: 100%;
  display: flex;
  flex-direction: column;
  gap: 10px;

  & leo-dropdown {
    width: 100%;
  }
`

export const ErrorText = styled.span`
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  color: ${(p) => p.theme.color.errorText};
  margin-bottom: 10px;
`

export const LoadingWrapper = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  width: 100%;
  height: 100%;
`

export const LoadIcon = styled(LoaderIcon)<Partial<StyleProps>>`
  color: ${(p) => p.theme.color.interactive08};
  height: ${(p) => (p.size === 'small' ? '25px' : '70px')};
  width: ${(p) => (p.size === 'small' ? '25px' : '70px')};
  opacity: 0.4;
`

export const NoSearchResultText = styled.div`
  font-family: Poppins;
  font-size: 12px;
  text-align: center;
  letter-spacing: 0.01em;
  width: 100%;
  margin-top: 16px;
  color: ${(p) => p.theme.color.text02};
`

export const Instructions = styled.div<{ mode: 'info' | 'success' | 'error' }>`
  display: flex;
  align-items: center;
  justify-content: center;
  color: ${(p) =>
    p.mode === 'info'
      ? leo.color.text.primary
      : p.mode === 'success'
      ? leo.color.systemfeedback.successText
      : leo.color.systemfeedback.errorText};
  text-align: center;
  font: ${leo.font.large.regular};
  gap: 8px;
`

export const Bold = styled.span`
  font-weight: 600;
`

export const AccountCheckbox = styled(Checkbox)`
  --checkbox-box-size: 24px;
`

export const AccountListContainer = styled.div`
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  padding: 0px;
  width: 100%;
  border-radius: 16px;
  border: 1px solid ${leo.color.divider.subtle};
  margin-top: 8px;
`

export const AccountListHeader = styled.div`
  display: flex;
  justify-content: space-between;
  align-items: center;
  width: 100%;

  color: ${leo.color.text.tertiary};
  font: ${leo.font.small.semibold};
  border-bottom: 1px solid ${leo.color.divider.subtle};
  padding: 16px;
`

export const AccountListContent = styled.div`
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  padding: 0px;
  width: 100%;
  height: 185px;
  overflow-y: auto;
`

export const DropdownLabel = styled.div`
  font-family: 'Poppins';
  font: ${leo.font.small.semibold};
  color: ${leo.color.text.primary};
`

export const HelpLink = styled.a`
  font-family: Poppins;
  font: ${leo.font.small.semibold};
  color: ${leo.color.text.interactive};
  text-decoration: none;
`

export const Divider = styled.div`
  width: 100%;
  height: 1px;
  background: ${leo.color.divider.subtle};
`
