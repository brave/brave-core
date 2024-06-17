// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'

import { WalletButton, Row } from '../style'
import { CaratStrongDownIcon } from 'brave-ui/components/icons'
import { layoutPanelWidth } from '../../desktop/wallet-page-wrapper/wallet-page-wrapper.style'

// graphics
import NftPlaceholderIcon from '../../../assets/svg-icons/nft-placeholder.svg'

export const AdvancedButton = styled(WalletButton)`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: row;
  cursor: pointer;
  outline: none;
  background: none;
  border: none;
  padding: 0px;
`

export const AdvancedIcon = styled(CaratStrongDownIcon)<
  Partial<{ rotated: boolean }>
>`
  width: 18px;
  height: 18px;
  color: ${(p) => p.theme.color.interactive07};
  transform: ${(p) => (p.rotated ? 'rotate(180deg)' : 'rotate(0deg)')};
  margin-right: 10px;
`

export const ButtonRow = styled(Row)`
  padding: 32px;
  @media screen and (max-width: ${layoutPanelWidth}px) {
    padding: 24px 16px;
  }

  & > * {
    flex: 1;
  }
`

export const ErrorText = styled.span`
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  color: ${(p) => p.theme.color.errorText};
  margin-bottom: 10px;
  text-align: left;
`

export const DividerRow = styled.div`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: space-between;
  width: 100%;
  margin-top: 10px;
`

export const SubDivider = styled.div`
  width: 100%;
  height: 2px;
  background-color: ${(p) => p.theme.color.divider01};
`

export const DividerText = styled.span`
  font-family: Poppins;
  font-size: 15px;
  line-height: 20px;
  letter-spacing: 0.04em;
  font-weight: 600;
  margin-bottom: 10px;
  color: ${(p) => p.theme.color.text03};
`
export const FormWrapper = styled.div`
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  justify-content: flex-start;
  gap: 24px;
  width: 100%;
  height: 100%;
  margin-top: 10px;
  box-sizing: border-box;
  overflow: auto;
  padding: 0px 32px;
`

export const FormRow = styled.div`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: space-between;
  width: 100%;
  gap: 17px;
`

export const FormColumn = styled.div`
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  justify-content: center;
  width: 49%;
  gap: 16px;
  & > * {
    width: 100%;
  }
`

export const FullWidthFormColumn = styled(FormColumn)`
  width: 100%;
`

export const InputLabel = styled.span`
  color: ${leo.color.text.primary};
  font: ${leo.font.small.semibold};
`

export const TokenNamePreviewText = styled.span`
  color: ${leo.color.text.primary};
  font: ${leo.font.default.regular};
  text-align: center;
`

export const TokenTickerPreviewText = styled.span`
  color: ${leo.color.text.primary};
  font: ${leo.font.default.semibold};
  text-align: center;
`

export const ButtonRowSpacer = styled.div`
  display: flex;
  width: 100% auto;
  margin-top: 14px;
`

export const PreviewImageContainer = styled.div`
  height: 96px;
  width: unset;
  position: relative;
  background: center / contain no-repeat url(${NftPlaceholderIcon});
  border-radius: 8px;
`
