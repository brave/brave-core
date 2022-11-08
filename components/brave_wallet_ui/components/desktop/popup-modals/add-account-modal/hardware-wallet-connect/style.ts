// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'
import { LoaderIcon } from 'brave-ui/components/icons'
import LedgerLogo from '../../../../../assets/svg-icons/ledger-logo.svg'
import TrezorLogo from '../../../../../assets/svg-icons/trezor-logo.svg'
import { DisclaimerWrapper as DisclaimerWrapperBase } from '../style'
import { WalletButton } from '../../../../shared/style'

interface StyleProps {
  isSelected: boolean
  size: 'big' | 'small'
}

export const HardwareTitle = styled.span`
  font-family: Poppins;
  font-size: 14px;
  line-height: 20px;
  font-weight: 600;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.text02};
  margin-bottom: 25px;
`

export const HardwareButtonRow = styled.div`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: space-between;
  width: 260px;
  margin-bottom: 35px;
`

export const HardwareButton = styled(WalletButton) <Partial<StyleProps>>`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: row;
  cursor: ${(p) => p.disabled ? 'not-allowed' : 'pointer'};
  outline: none;
  background: none;
  border: ${(p) => (p.isSelected ? `2px solid ${p.theme.color.infoBorder}` : `1px solid ${p.theme.color.disabled}`)};
  background-color: ${(p) => (p.isSelected ? p.theme.color.infoBackground : p.theme.color.background02)};
  border-radius: 10px;
  width: 125px;
  height: 55px;
`

export const LedgerIcon = styled.div`
  width: 93px;
  height: 23px;
  background-color: ${(p) => p.theme.color.interactive07};
  -webkit-mask-image: url(${LedgerLogo});
  mask-image: url(${LedgerLogo});
`

export const TrezorIcon = styled.div`
  width: 105px;
  height: 33px;
  background-color: ${(p) => p.theme.color.interactive07};
  -webkit-mask-image: url(${TrezorLogo});
  mask-image: url(${TrezorLogo});
`

export const HardwareInfoRow = styled.div`
  display: flex;
  flex-direction: row;
  align-items: flex-start;
  justify-content: flex-start;
  margin-bottom: 35px;
`

export const HardwareInfoColumn = styled.div`
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  justify-content: flex-start;
  margin-left: 10px;
`

export const ConnectingButton = styled(WalletButton)`
  display: flex;
  flex-direction: row;
  justify-content: center;
  align-items: center;
  padding: 10px 22px;
  border: 1px solid ${(p) => p.theme.color.interactive08};
  box-sizing: border-box;
  border-radius: 48px;
  background-color: transparent;
`

export const ConnectingButtonText = styled.span`
  font-family: Poppins;
  font-style: normal;
  font-weight: 600;
  font-size: 13px;
  line-height: 20px;
  text-align: center;

  /* Light Theme/Brand/interactive07 */
  color: ${(p) => p.theme.color.interactive07};

  /* Inside Auto Layout */
  flex: none;
  order: 1;
  flex-grow: 0;
  margin: 0px 8px;
`

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

export const HardwareWalletAccountsList = styled.div`
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
`

export const HardwareWalletAccountListItemRow = styled.div`
  flex: 1;
  flex-direction: row;
  /* Body Light Theme/14pt Poppins Regular 400 */
  font-family: Poppins;
  font-style: normal;
  font-weight: normal;
  font-size: 14px;
  line-height: 20px;
  display: flex;
  align-items: center;
  letter-spacing: 0.01em;

  /* Light Theme/Text/text01 */
  color: ${(p) => p.theme.color.text01};

  justify-content: space-between;
  padding-left: 10px;
  padding-right: 10px;
`

export const AddressBalanceWrapper = styled.div`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: flex-start;
  width: 140px;
  height: 100%;
`

export const ButtonsContainer = styled.div`
  display: flex;
  flex-direction: row;
  button:first-child {
    margin-right: 10px;
  }
`

export const DisclaimerWrapper = styled(DisclaimerWrapperBase)`
  margin-bottom: 10px;
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
  width: 300px;
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

export const LoadIcon = styled(LoaderIcon) <Partial<StyleProps>>`
  color: ${p => p.theme.color.interactive08};
  height: ${(p) => p.size === 'small' ? '25px' : '70px'};
  width: ${(p) => p.size === 'small' ? '25px' : '70px'};
  opacity: .4;
`

export const NoSearchResultText = styled.div`
  font-family: Poppins;
  font-size: 12px;
  text-align: center;
  letter-spacing: 0.01em;
  width: 100%;
  margin-top: 16px;

  /* Light Theme/Text/text02 */
  color: ${(p) => p.theme.color.text02};
`
