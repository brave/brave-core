// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'
import Icon from '@brave/leo/react/icon'
import ProgressRing from '@brave/leo/react/progressRing'

// Types
import { BraveWallet } from '../../../constants/types'

// Shared Styles
import {
  Text,
  AssetIconFactory,
  AssetIconProps,
  Column
} from '../../shared/style'

export const PortfolioTransactionItemWrapper = styled.div<{
  isFocused?: boolean
}>`
  display: flex;
  align-items: center;
  justify-content: space-between;
  flex-direction: row;
  width: 100%;
  position: relative;
  transition: background-color 300ms ease-out;
  background-color: ${(p) =>
    p.isFocused ? `${leo.color.page.background}` : 'none'};
  padding: 12px 16px;
  border-radius: 12px;
  border: 1px solid ${leo.color.divider.subtle};
  cursor: pointer;
  &:hover {
    background-color: ${leo.color.page.background};
  }
`

export const TransactionTypeIcon = styled(Icon)`
  --leo-icon-size: 12px;
  color: ${leo.color.icon.default};
  margin-right: 4px;
`

export const DateText = styled(Text)`
  line-height: 18px;
  color: ${leo.color.text.tertiary};
  margin-right: 8px;
`

export const TransactionTypeText = styled(Text)`
  line-height: 18px;
  color: ${leo.color.text.tertiary};
  text-transform: capitalize;
  margin-right: 4px;
`

export const IntentAddressText = styled(Text)`
  line-height: 18px;
  color: ${leo.color.text.tertiary};
`

const assetIconProps = {
  width: '32px',
  height: 'auto'
}
export const AssetIcon = AssetIconFactory<AssetIconProps>(assetIconProps)

const swapIconProps = {
  width: '24px',
  height: 'auto'
}
export const SwapIcon = AssetIconFactory<AssetIconProps>(swapIconProps)

export const SellIconPlaceholder = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  width: 24px;
  height: 24px;
  border-radius: 100%;
  background-color: ${leo.color.neutral[40]};
  color: ${leo.color.container.background};
`

export const BuyIconPlaceholder = styled(SellIconPlaceholder)`
  background-color: ${leo.color.neutral[20]};
  color: ${leo.color.icon.default};
`

export const SwapPlaceholderIcon = styled(Icon).attrs({
  name: 'crypto-wallets'
})`
  --leo-icon-size: 16px;
`

export const TokenNameText = styled(Text)`
  line-height: 24px;
  color: ${leo.color.text.primary};
`

export const TokenSymbolText = styled(Text)`
  line-height: 18px;
  color: ${leo.color.text.tertiary};
`

export const AssetValueText = styled(Text)`
  line-height: 24px;
  color: ${leo.color.text.primary};
`

export const FiatValueText = styled(Text)`
  line-height: 18px;
  color: ${leo.color.text.tertiary};
`

export const ArrowIconWrapper = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  background-color: ${leo.color.container.highlight};
  border-radius: 100%;
  width: 12px;
  height: 12px;
`

export const ArrowIcon = styled(Icon).attrs({
  name: 'carat-right'
})`
  --leo-icon-size: 12px;
  color: ${leo.color.icon.default};
`

export const IconWrapper = styled(Column)`
  position: relative;
`

export const SwapIconsWrapper = styled(Column)`
  position: relative;
  width: 32px;
  height: 32px;
`

export const SwapSellIcon = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  position: absolute;
  top: 0px;
  right: 0px;
`

export const SwapBuyIcon = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  position: absolute;
  bottom: 0px;
  left: 0px;
`

export const NetworkIconWrapper = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  position: absolute;
  bottom: -2px;
  right: -2px;
  width: 18px;
  height: 18px;
  background-color: ${leo.color.container.background};
  border-radius: 100%;
`

export const StatusBubble = styled.div<{
  status: BraveWallet.TransactionStatus
}>`
  display: flex;
  align-items: center;
  justify-content: center;
  border-radius: 100%;
  border: 2px solid ${leo.color.container.background};
  width: 16px;
  height: 16px;
  position: absolute;
  top: -5px;
  left: -2px;
  background-color: ${(p) =>
    p.status === BraveWallet.TransactionStatus.Error ||
    p.status === BraveWallet.TransactionStatus.Dropped
      ? leo.color.systemfeedback.errorIcon
      : p.status === BraveWallet.TransactionStatus.Unapproved
      ? leo.color.neutral[20]
      : leo.color.systemfeedback.infoIcon};
`

export const LoadingIcon = styled(ProgressRing)`
  --leo-progressring-size: 10px;
  --leo-progressring-color: ${leo.color.white};
`

export const StatusIcon = styled(Icon)`
  --leo-icon-size: 14px;
  color: ${leo.color.white};
`

export const BalancesColumn = styled(Column)`
  flex-wrap: wrap;
`
