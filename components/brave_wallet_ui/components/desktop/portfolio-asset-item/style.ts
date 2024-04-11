// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css'
import Icon from '@brave/leo/react/icon'
import ProgressRing from '@brave/leo/react/progressRing'

// Shared Styles
import {
  AssetIconProps,
  AssetIconFactory,
  WalletButton,
  Text,
  Column,
  Row
} from '../../shared/style'
import {
  layoutPanelWidth //
} from '../wallet-page-wrapper/wallet-page-wrapper.style'

export const HoverArea = styled.div<{
  noHover?: boolean
  isGrouped?: boolean
}>`
  display: flex;
  align-items: center;
  justify-content: space-between;
  flex-direction: row;
  width: 100%;
  padding: 12px;
  border-radius: var(--hover-area-border-radius);
  transition: background-color 300ms ease-out;
  &:hover {
    background-color: ${(p) =>
      p.noHover ? 'none' : leo.color.page.background};
  }
  @media screen and (max-width: ${layoutPanelWidth}px) {
    padding: ${(p) => (p.isGrouped ? 12 : 8)}px;
  }
`

export const Button = styled(WalletButton)<{
  disabled: boolean
  rightMargin?: number
}>`
  display: flex;
  align-items: center;
  justify-content: space-between;
  flex-direction: row;
  width: 100%;
  cursor: ${(p) => (p.disabled ? 'default' : 'pointer')};
  outline: none;
  background: none;
  border: none;
  margin-right: ${(p) => (p.rightMargin ? p.rightMargin : 0)}px;
  padding: 0px;
`

export const NameAndIcon = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: row;
  text-align: left;
`

export const AssetName = styled(Text)`
  line-height: 22px;
  color: ${leo.color.text.primary};
`

export const BalanceColumn = styled.div`
  display: flex;
  align-items: flex-end;
  justify-content: center;
  flex-direction: column;
  text-align: right;
`

export const FiatBalanceText = styled(Text)`
  line-height: 20px;
  color: ${leo.color.text.secondary};
`

export const AssetBalanceText = styled(Text)`
  line-height: 22px;
  color: ${leo.color.text.primary};
`

// Construct styled-component using JS object instead of string, for editor
// support with custom AssetIconFactory.
//
// Ref: https://styled-components.com/docs/advanced#style-objects
const assetIconProps = {
  width: '32px',
  height: 'auto'
}
export const AssetIcon = AssetIconFactory<AssetIconProps>(assetIconProps)

export const NameColumn = styled.div`
  display: flex;
  align-items: flex-start;
  justify-content: center;
  flex-direction: column;
`

export const Spacer = styled.div`
  display: flex;
  height: 4px;
`

export const NetworkDescriptionText = styled(Text)`
  line-height: 20px;
  color: ${leo.color.text.secondary};
`

export const AssetMenuWrapper = styled.div`
  position: relative;
`

export const AssetMenuButton = styled(WalletButton)`
  display: flex;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  outline: none;
  background: none;
  pointer-events: auto;
  border: none;
  margin: 0px;
  padding: 0px;
`

export const AssetMenuButtonIcon = styled(Icon).attrs({
  name: 'more-vertical'
})`
  --leo-icon-size: 24px;
  color: ${leo.color.icon.default};
`

export const Wrapper = styled(Column)<{
  showBorder?: boolean
  isGrouped?: boolean
}>`
  border-radius: ${(p) => (p.showBorder ? 16 : 0)}px;
  border: ${(p) =>
    p.showBorder ? `1px solid ${leo.color.divider.subtle}` : 'none'};
  --hover-area-border-radius: ${(p) => (p.isGrouped ? '0px' : '10px')};
  &:last-child {
    --hover-area-border-radius: ${(p) =>
      p.isGrouped ? '0px 0px 10px 10px' : '10px'};
  }
`

export const InfoBar = styled(Row)`
  background-color: ${leo.color.systemfeedback.infoBackground};
  border-radius: 12px;
  padding: 8px 16px;
`

export const InfoText = styled(Text)`
  line-height: 22px;
  color: ${leo.color.systemfeedback.infoText};
`

export const LoadingRing = styled(ProgressRing)`
  --leo-progressring-size: 20px;
`
