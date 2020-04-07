/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'
import { BatColorIcon } from 'brave-ui/components/icons'
import Button, { Props as ButtonProps } from 'brave-ui/components/buttonsIndicators/button'
import { ComponentType } from 'react'

export const Content = styled.div`
  display: flex;
  margin: 12px 0 6px 0;
`

export const WalletInfoPanel = styled.div<{ hasSufficientFunds?: boolean }>`
  flex-grow: 1;
  position: relative;
  top: 0;
  left: 0;

  > * {
    opacity: ${p => p.hasSufficientFunds ? '1' : '.5'};
  }
`

export const BatIcon = styled(BatColorIcon)`
  height: 24px;
  width: 24px;
  position: relative;
  top: -4px;
  left: 0;
  opacity: 1;
  vertical-align: middle;
  display: inline-block;
`

export const ActionPanel = styled.div`
  flex-grow: 0;
  padding-right: 4px;
  max-width: 185px;
`

export const ActionPanelButton = styled(Button as ComponentType<ButtonProps>)`
  margin-top: -9px;
  padding-left: 21px;
  padding-right: 21px;
`

export const BatAmount = styled.span`
  font-size: 22px;
  padding-left: 8px;
`

export const BatSymbol = styled.span`
  padding-left: 5px;
  font-size: 18px;
`

export const ExchangeAmount = styled.span`
  font-size: 18px;
  color: ${p => p.theme.palette.grey600};
  padding-left: 14px;
`

export const LastUpdated = styled.div`
  font-family: ${p => p.theme.fontFamily.body};
  font-size: 12px;
  color: ${p => p.theme.palette.grey500};
  padding-top: 7px;
  padding-left: 2px;
  opacity: 1;
`

export const NotEnoughFunds = styled.div`
  font-family: ${p => p.theme.fontFamily.body};
  font-size: 14px;
  color: ${p => p.theme.palette.grey600};
  padding: 2px 0 0 5px;
`

export const TermsOfSale = styled.div`
  font-family: ${p => p.theme.fontFamily.body};
  font-size: 12px;
  text-align: center;

  a {
    font-weight: bold;
    color: ${p => p.theme.palette.black};
  }
`
