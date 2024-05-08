// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'
import Icon from '@brave/leo/react/icon'
import { WalletButton } from '../../shared/style'

export const StyledWrapper = styled.div<{
  isRewardsAccount: boolean
}>`
  cursor: ${(p) => (p.isRewardsAccount ? 'default' : 'pointer')};
  display: flex;
  align-items: center;
  justify-content: space-between;
  flex-direction: row;
  width: 100%;
  margin-bottom: 8px;
  padding-right: 8px;
  position: relative;
  border-radius: 12px;
  transition: background-color 300ms ease-out;
  &:hover {
    background-color: ${(p) =>
      p.isRewardsAccount ? 'transparent' : leo.color.page.background};
  }
`

export const AccountMenuWrapper = styled.div`
  position: relative;
`

export const AccountMenuButton = styled(WalletButton)`
  display: flex;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  outline: none;
  background: none;
  pointer-events: auto;
  border: none;
`

export const AccountMenuIcon = styled(Icon).attrs({
  name: 'more-vertical'
})`
  --leo-icon-size: 24px;
  color: ${leo.color.icon.default};
`

export const AccountButton = styled(WalletButton)`
  cursor: pointer;
  display: flex;
  align-items: center;
  justify-content: space-between;
  flex-direction: row;
  width: 100%;
  outline: none;
  background: none;
  border: none;
  color: none;
  margin: 0px;
  padding: 8px 0px 8px 8px;
  &:disabled {
    cursor: default;
  }
`
