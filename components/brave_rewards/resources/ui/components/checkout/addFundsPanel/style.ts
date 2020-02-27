/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'
import { BatColorIcon } from 'brave-ui/components/icons'
import Button, { Props as ButtonProps } from 'brave-ui/components/buttonsIndicators/button'
import { ComponentType } from 'react'

export const Subtitle = styled.div`
  text-align: center;
  font-family: ${p => p.theme.fontFamily.body};
  font-size: 15px;
  margin: -10px 0 22px 0;
`

export const CurrentBalance = styled.div`
  padding: 15px 8px 0;
  border-top: solid 1px ${p => p.theme.color.separatorLine};
  display: flex;
  justify-content: space-between;
  font-size: 12px;
  margin-bottom: -12px;
`

export const CurrentBalanceBat = styled.span`
  padding-left: 7px;
  font-size: 16px;
  font-weight: 600;
`

export const CurrentBalanceConverted = styled.span`
  padding-left: 7px;
  color: ${p => p.theme.palette.grey600};
  font-size: 14px;
`

export const CurrentBalanceNeeded = styled.div`
  text-align: right;
  color: ${p => p.theme.palette.green700};
  font-weight: 500;
  font-size: 14px;
`

export const PurchaseButtonRow = styled.div`
  margin: 20px 0 0;
  padding-right: 7px;
  display: flex;
  align-items: center;
  justify-content: space-between;
`

export const AddFundsButton = styled(Button as ComponentType<ButtonProps>)`
  padding-left: 42px;
  padding-right: 42px;
`

export const ExchangeRateDisplay = styled.div`
  font-size: 12px;
  color: ${p => p.theme.palette.grey600};
  text-align: right;
  margin-top: -2px;
`

export const BatIcon = styled(BatColorIcon)`
  height: 18px;
  width: 18px;
  vertical-align: text-bottom;
`

export const AmountOptionList = styled.div`
  margin-top: 22px;
  display: flex;
  justify-content: center;
  flex-wrap: wrap;
`

export const AmountOptionContainer = styled.div`
  margin: 0 10px 12px;
  text-align: center;
`

interface AmountOptionButtonProps extends ButtonProps {
  selected?: boolean
}

export const AmountOptionButton = styled(Button as ComponentType<AmountOptionButtonProps>)`
  min-width: 98px;
  background: ${p => p.selected ? p.theme.color.brandBat : 'transparent'};
  border-color: ${p => p.theme.color.brandBat};
  margin-bottom: 7px;
  color: ${p => p.selected ? p.theme.palette.white : p.theme.color.brandBat};
  font-size: 15px;
`

export const AmountOptionExchange = styled.div`
  font-family: ${p => p.theme.fontFamily.body};
  font-size: 11px;
  color: ${p => p.theme.palette.grey600};
`

export const ChargeSummary = styled.div`
  padding: 8px 7px 5px;
  border-top: solid 1px ${p => p.theme.color.separatorLine};
  display: grid;
  grid-template-columns: auto minmax(auto, 84px);
  text-align: right;
`

export const ChargeSummaryTotal = styled.div`
  padding-top: 10px;
  font-size: 16px;
`

export const ChargeSummaryTotalAmount = styled.div`
  padding-top: 10px;
  font-size: 16px;
  font-weight: 500;
`

export const TermsOfSale = styled.div`
  font-family: ${p => p.theme.fontFamily.body};
  font-size: 12px;
  text-align: center;
  padding-top: 20px;

  a {
    font-weight: bold;
    color: ${p => p.theme.palette.black};
  }
`
