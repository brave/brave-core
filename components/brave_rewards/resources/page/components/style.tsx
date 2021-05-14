/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
import * as React from 'react'
import styled from 'styled-components'
import Card, { CardProps } from 'brave-ui/components/layout/card'

export const QRBoxWrapper = styled('div')<{}>`
  width: 100%;
  margin: 0 0 24px;
`

const CustomCard: React.FC<CardProps> = (props) =>
  <Card emphasis={'60'} {...props} />

export const QRBoxStyle = styled(CustomCard)`
  font-size: 14px;
  display: flex;
  justify-content: space-between;
  align-items: center;
  width: 100%;
  box-shadow: 0 0;
`

export const QRText = styled('div')<{}>`
  font-size: 15px;
  color: ${p => p.theme.color.text};
  padding: 0 10px 0 0;
  line-height: 1.5;
`

export const TourPromoWrapper = styled('div')<{}>`
  margin-top: 30px;
`

export const ArrivingSoon = styled.div`
  background: rgba(93, 181, 252, 0.2);
  border-radius: 4px;
  margin-bottom: 8px;
  padding: 4px;
  color: var(--brave-palette-neutral700);
  text-align: center;
  font-size: 12px;
  line-height: 22px;

  span.amount {
    color: var(--brave-palette-neutral900);
    font-weight: 600;
    font-size: 14px;
    line-height: 24px;
  }

  .icon {
    height: 16px;
    width: auto;
    fill: var(--brave-palette-blue500);
    vertical-align: middle;
    margin-bottom: 3px;
    margin-right: 8px;
  }
`
