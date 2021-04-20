/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const Container = styled.div`
  padding: 15px 0px 15px;
  border-top: solid 1px ${p => p.theme.color.separatorLine};
  border-bottom: solid 1px ${p => p.theme.color.separatorLine};
`
export const OrderTotal = styled.span`
  font-size: 20px;
  text-align: left;
  vertical-align: center;
  color: ${p => p.theme.palette.blurple600};
  font-weight: 500;
`

export const BatAmount = styled.span`
  float: right;
  color: #000000;
  font-size: 20px;
  font-weight: 500;
`

export const BatSymbol = styled.span`
  text-align: right;
  padding-left: 5px;
  font-size: 20px;
  font-weight: normal;
`

export const ExchangeAmount = styled.span`
  float: right;
  margin: 1px 0px 0px 5px;
  font-size: 18px;
  color: ${p => p.theme.palette.grey600};
`
