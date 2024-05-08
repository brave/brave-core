// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'
import Icon from '@brave/leo/react/icon'
import { LoaderIcon } from 'brave-ui/components/icons'
import { WalletButton, Row } from '../../../shared/style'
import { layoutPanelWidth } from '../../wallet-page-wrapper/wallet-page-wrapper.style'

export const VirtualListStyle = {
  flex: 1
}

export const StyledWrapper = styled.div`
  display: flex;
  box-sizing: border-box;
  flex-direction: column;
  align-items: center;
  justify-content: flex-start;
  flex: 1;
  width: 100%;
`

export const LoadingWrapper = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  width: 100%;
  flex: 1;
`

export const LoadIcon = styled(LoaderIcon)`
  color: ${leo.color.icon.interactive};
  height: 70px;
  width: 70px;
  opacity: 0.4;
`

export const Divider = styled.div`
  width: 100%;
  height: 2px;
  background-color: ${(p) => p.theme.color.divider01};
`

export const NoAssetButton = styled(WalletButton)`
  cursor: pointer;
  outline: none;
  border: none;
  background: none;
  padding: 0px;
  margin: 0px;
  font-family: Poppins;
  font-size: 14px;
  letter-spacing: 0.4px;
  line-height: 20px;
  font-weight: 600;
  color: ${leo.color.text.interactive};
`

export const TitleRow = styled(Row)`
  padding: 12px 40px 12px 32px;
  @media screen and (max-width: ${layoutPanelWidth}px) {
    padding: 12px 24px 12px 16px;
  }
`

export const ButtonRow = styled(Row)`
  padding: 32px;
  @media screen and (max-width: ${layoutPanelWidth}px) {
    padding: 24px 16px;
  }
`

export const ListWrapper = styled.div`
  flex: 1;
  box-sizing: border-box;
  width: 100%;
`

export const EmptyStateWrapper = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  box-sizing: border-box;
  width: 100%;
  height: 100%;
  padding: 0px 74px;
`

export const AddIcon = styled(Icon).attrs({
  name: 'plus-add'
})`
  --leo-icon-size: 18px;
  color: ${leo.color.icon.interactive};
  margin-right: 8px;
`

export const AddButtonText = styled.span`
  color: ${leo.color.text.interactive};
`

export const InfoIcon = styled(Icon).attrs({
  name: 'info-outline'
})`
  --leo-icon-size: 40px;
  color: ${leo.color.icon.default};
  margin-bottom: 16px;
`
