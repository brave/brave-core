/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

const batOutline = require('./assets/batOutline.svg')

export const StyledWrapper = styled.div`
  text-align: center;
  border-radius: 8px;
  font-family: "Poppins", sans-serif;
  background: linear-gradient(-211.85275762473051deg, #392DD1 0%, rgba(255,26,26,0.53) 100%) #7D7BDC;
` as any

export const StyledInnerWrapper = styled.div`
  margin: 0 auto;
  display: inline-block;
  padding: 30px;
  background: url(${batOutline}) no-repeat top;
` as any

export const StyledHeaderText = styled.p`
  width: 261px;
  color: #FFFFFF;
  font-size: 16px;
  font-weight: 300;
  letter-spacing: 0.16px;
  line-height: 22px;
  margin: 0 auto;
  opacity: 0.5;
` as any

export const StyledBatLogo = styled.span`
  display: block;
  padding: 20px 0px 15px 0px;
` as any

export const StyledTitle = styled.h1`
  color: #FFFFFF;
  font-size: 28px;
  font-weight: 400;
  letter-spacing: 0.16px;
  line-height: 22px;
  margin: 0;
  display: inline-block;
` as any

export const StyledDescText = styled.p`
  color: #FFFFFF;
  font-size: 16px;
  font-weight: normal;
  letter-spacing: 0;
  line-height: 24px;
  opacity: 0.5;
  margin: 20px auto;
  text-align: center;
  width: 295px;
` as any

export const StyledButtonWrapper = styled.div`
  margin: 0px 0px 40px 0px;
` as any

export const StyledFooterText = styled.p`
  color: #73CBFF;
  font-size: 14px;
  font-weight: 500;
  letter-spacing: -0.19px;
  line-height: 21px;
  margin: 0 auto;
` as any

export const StyledTrademark = styled.span`
  display: inline-block;
  vertical-align: text-top;
  margin-top: -12px;
  color: #FFF;
  font-size: 9px;
  font-weight: 500;
  opacity: 0.7;
` as any
