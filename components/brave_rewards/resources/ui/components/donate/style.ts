/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'
import * as CSS from 'csstype'
import { DonateType, Props } from './index'

interface Theme {
  paddingBox: CSS.PaddingProperty<1>
  sendBgColor: CSS.Color
  disabledSendColor: CSS.Color
  paddingSend: CSS.PaddingProperty<1>
  paddingFunds: CSS.PaddingProperty<1>
}

const theme: Record<DonateType, Theme> = {
  big: {
    paddingBox: '0 19px 0 55px',
    sendBgColor: '#4c54d2',
    disabledSendColor: '#3e45b2',
    paddingSend: '16px 19px 16px 55px',
    paddingFunds: '13px 12px 13px 24px'
  },
  small: {
    paddingBox: '0 0 0 23px',
    sendBgColor: '#392dd1',
    disabledSendColor: '#1a22a8',
    paddingSend: '13px 0 13px 51px',
    paddingFunds: '13px 0 14px 25px'
  }
}

export const StyledWrapper = styled.div`
  position: relative;
  font-family: Poppins, sans-serif;
` as any

export const StyledContent = styled.div`
  padding: ${(p: Props) => theme[p.donateType].paddingBox};
` as any

export const StyledDonationTitle = styled.div`
  font-size: 16px;
  font-weight: 600;
  line-height: 1.75;
  color: #fff;
  margin-bottom: 14px;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
  max-width: 167px;
` as any

export const StyledSend = styled.div`
  background: ${(p: {disabled: boolean, donateType: DonateType}) => theme[p.donateType].sendBgColor};
  font-size: 13px;
  font-weight: 600;
  letter-spacing: 0.2px;
  color: ${(p: {disabled: boolean, donateType: DonateType}) => p.disabled ? theme[p.donateType].disabledSendColor : '#fff'};
  padding: ${(p: {disabled: boolean, donateType: DonateType}) => theme[p.donateType].paddingSend};
  text-transform: uppercase;
` as any

export const StyledIconSend = styled.span`
  vertical-align: middle;
  display: inline-block;
  margin-right: 18px;
` as any

export const StyledFunds = styled.div`
  font-family: Muli, sans-serif;
  font-size: 13px;
  font-weight: 300;
  line-height: 1.69;
  color: #fff;
  padding: ${(p: {donateType: DonateType}) => theme[p.donateType].paddingFunds};
  background: #1b1d2f;
  display: flex;
  position:absolute;
  bottom: 0;
  left: 0;
  z-index: 10;
  width: 100%;

  a {
    color: #6cc7fd;
    text-decoration: none;
  }
` as any

export const StyledIconFace = styled.div`
  flex-basis: 26px;
  margin-right: 9px;
` as any

export const StyledFundsText = styled.div`
  flex: 1;
  margin-right: 9px;
` as any
