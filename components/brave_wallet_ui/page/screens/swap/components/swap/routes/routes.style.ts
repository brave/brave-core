// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'
import Icon from '@brave/leo/react/icon'

// Shared Styles
import {
  WalletButton,
  Column,
  Row,
  AssetIconFactory,
  AssetIconProps
} from '../../../../../../components/shared/style'
import { LPIcon } from '../../shared-swap.styles'
import {
  layoutPanelWidth //
} from '../../../../../../components/desktop/wallet-page-wrapper/wallet-page-wrapper.style'

export const StyledWrapper = styled(Column)`
  overflow: hidden;
  @media screen and (max-width: ${layoutPanelWidth}px) {
    max-height: 500px;
  }
`

export const OptionButton = styled(WalletButton)<{
  isSelected: boolean
}>`
  cursor: pointer;
  display: flex;
  flex-direction: column;
  justify-content: center;
  align-items: center;
  background: none;
  background-color: none;
  outline: solid 2px
    ${(p) =>
      p.isSelected ? leo.color.button.background : leo.color.divider.subtle};
  border: none;
  border-radius: ${leo.radius.m};
  width: 100%;
  padding: 12px;
`

export const IconsWrapper = styled(Column)`
  position: relative;
`

export const AssetIcon = AssetIconFactory<AssetIconProps>({
  width: '32px',
  height: 'auto'
})

export const NetworkIconWrapper = styled(Column)`
  position: absolute;
  bottom: -3px;
  right: -3px;
  background-color: ${leo.color.container.background};
  border-radius: 100%;
`

export const CaratDownIcon = styled(Icon).attrs({
  name: 'carat-down'
})<{ isExpanded: boolean }>`
  --leo-icon-size: 16px;
  color: ${leo.color.icon.default};
  transition-duration: 0.3s;
  transform: ${(p) => (p.isExpanded ? 'rotate(180deg)' : 'unset')};
`

export const GasFeeBubble = styled(Row)`
  border-radius: ${leo.radius.s};
  background: ${leo.color.container.interactive};
`

export const GasIcon = styled(Icon).attrs({
  name: 'search-fuel-tank'
})`
  --leo-icon-size: 14px;
  color: ${leo.color.icon.default};
  margin-right: 2px;
`

export const StepsWrapper = styled(Column)`
  position: relative;
  overflow: hidden;
`

export const LPIconWrapper = styled(Column)`
  background-color: ${leo.color.container.background};
  border-radius: 100%;
  border: 2px dashed ${leo.color.divider.strong};
  position: relative;
`

export const ProviderIcon = styled(LPIcon)`
  border: 2px solid ${leo.color.container.background};
  position: absolute;
  bottom: -2px;
  right: -2px;
  border-radius: ${leo.radius.full};
`

export const Lines = styled.div`
  display: flex;
  height: 100%;
  border-left: 2px dashed ${leo.color.divider.strong};
  position: absolute;
  left: 15px;
`

export const PercentBubble = styled(Column)`
  border-radius: ${leo.radius.full};
  background: ${leo.color.page.background};
`

export const Triangle = styled.div`
  width: 0;
  height: 0;
  border-left: 4px solid transparent;
  border-right: 4px solid transparent;
  border-bottom: 6px solid ${leo.color.divider.strong};
`

export const Dot = styled.div`
  width: 6px;
  height: 6px;
  border-radius: ${leo.radius.full};
  background-color: ${leo.color.divider.strong};
`

export const ArrowIcon = styled(Icon).attrs({
  name: 'arrow-right'
})`
  --leo-icon-size: 14px;
  color: ${leo.color.icon.default};
  margin: 0px 4px 0px 4px;
`
