// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'
import { ArrowRightIcon, LoaderIcon } from 'brave-ui/components/icons'
import * as leo from '@brave/leo/tokens/css/variables'
import Icon from '@brave/leo/react/icon'

import LinkIcon from '../../../assets/svg-icons/link-icon.svg'
import { WarningBoxIcon } from '../shared-panel-styles'

// Shared Styles
import {
  AssetIconProps,
  AssetIconFactory,
  Text,
  WalletButton,
  styledScrollbarMixin,
} from '../../shared/style'

export const StyledWrapper = styled.div`
  display: flex;
  flex: 1;
  width: 100%;
  flex-direction: column;
  align-items: center;
  justify-content: space-between;
  background-color: ${leo.color.page.background};
  padding: 0px 16px;
`

export const TopRow = styled.div`
  display: flex;
  align-items: center;
  justify-content: space-between;
  flex-direction: row;
  width: 100%;
  padding: 15px 15px 0px 15px;
`

export const AccountCircleWrapper = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: row;
  position: relative;
  box-sizing: border-box;
  margin-bottom: 10px;
`

export const FromCircle = styled.div<{ orb: string }>`
  width: 54px;
  height: 54px;
  border-radius: 100%;
  background-image: url(${(p) => p.orb});
  background-size: cover;
`

export const ToCircle = styled.div<{ orb: string }>`
  width: 32px;
  height: 32px;
  border-radius: 100%;
  background-image: url(${(p) => p.orb});
  background-size: cover;
  position: absolute;
  left: 34px;
`

export const AccountNameText = styled(Text)`
  cursor: default;
`

export const TransactionAmountBig = styled(Text)`
  word-break: break-all;
  text-align: center;
`

export const TransactionFiatAmountBig = styled(Text)`
  margin-bottom: 10px;
  word-break: break-all;
  text-align: center;
`

export const MessageBox = styled.div<{ isDetails: boolean; width?: string }>`
  display: flex;
  align-items: flex-start;
  justify-content: 'flex-start';
  flex-direction: column;
  border: 1px solid ${leo.color.divider.subtle};
  box-sizing: border-box;
  border-radius: 4px;
  width: ${(p) => p.width ?? '90%'};
  height: 140px;
  padding: ${(p) => (p.isDetails ? '14px' : '4px 14px')};
  overflow-y: scroll;
  overflow-x: hidden;
  position: relative;
  word-break: break-all;
  ${styledScrollbarMixin}
`

export const TransactionTypeText = styled(Text)`
  word-break: break-all;
  text-align: left;
`

export const ArrowIcon = styled(ArrowRightIcon)`
  display: inline-block;
  width: 16px;
  height: 16px;
  color: ${leo.color.text.tertiary};
`

export const Divider = styled.div`
  width: 100%;
  height: 1px;
  background-color: ${leo.color.divider.subtle};
  border: 1px solid ${leo.color.divider.subtle};
  margin-top: 6px;
  margin-bottom: 6px;
`

export const SectionRow = styled.div`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: space-between;
  width: 100%;
  height: inherit;
`

export const SectionColumn = styled.div`
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  justify-content: center;
  width: 100%;
  height: inherit;
  margin-bottom: 5px;
`

export const EditButton = styled(WalletButton)`
  font: ${leo.font.small.semibold};
  letter-spacing: 0.01em;
  color: ${leo.color.button.background};
  background: none;
  cursor: pointer;
  outline: none;
  border: none;
  margin: 0px;
  padding: 0px;
`

export const TransactionText = styled(Text)`
  text-align: left;
  word-break: break-all;
`

export const AssetIcon = AssetIconFactory<AssetIconProps>({
  width: '40px',
  height: 'auto',
})

export const WarningIcon = styled(WarningBoxIcon)`
  width: 14px;
  height: 14px;
  margin-right: 6px;
`

export const LoadIcon = styled(LoaderIcon)`
  color: ${leo.color.neutral[30]};
  height: 25px;
  width: 24px;
  opacity: 0.4;
`

export const ContractButton = styled(WalletButton)`
  font: ${leo.font.default.semibold};
  display: flex;
  align-items: center;
  justify-content: center;
  letter-spacing: 0.01em;
  color: ${leo.color.button.background};
  background: none;
  cursor: pointer;
  outline: none;
  border: none;
  margin: 0px;
  padding: 0px;
`

export const InlineAddressButton = styled(ContractButton)`
  display: inline-flex;
  flex-direction: row;
  align-items: center;
  gap: 4px;
  text-align: left;
  vertical-align: center;
  color: ${leo.color.text.secondary};
  font: ${leo.font.xSmall.regular};
`

export const ExplorerIcon = styled.div`
  mask-image: url(${LinkIcon});
  width: 12px;
  height: 12px;
  margin-left: 8px;
  mask-size: contain;
  mask-repeat: no-repeat;
  background-color: ${leo.color.button.background};
`

export const WarningInfoCircleIcon = styled(Icon).attrs({
  name: 'warning-circle-outline',
})`
  --leo-icon-size: 16px;
  width: 16px;
  height: 16px;
  color: ${leo.color.systemfeedback.warningIcon};
`
