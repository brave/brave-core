/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'
import Heading from '../../../components/text/heading'

const batOutline = require('./assets/batOutline.svg')

export const StyledWrapper = styled<{}, 'div'>('div')`
  text-align: center;
  border-radius: 8px;
  font-family: "Poppins", sans-serif;
  background-image: linear-gradient(140deg, #392DD1 0%, #8E2995 100%);
` as any

export const StyledInnerWrapper = styled<{}, 'div'>('div')`
  margin: 0 auto;
  display: inline-block;
  padding: 35px 30px 25px;
  background: url(${batOutline}) no-repeat top;
` as any

export const StyledHeaderText = styled<{}, 'p'>('p')`
  width: 261px;
  color: #FFFFFF;
  font-size: 16px;
  font-weight: 300;
  letter-spacing: 0.16px;
  line-height: 22px;
  margin: 0 auto;
  opacity: 0.5;
  font-family: Muli, sans-serif;
` as any

export const StyledBatLogo = styled<{}, 'span'>('span')`
  display: block;
  margin: -10px auto 2px;
  width: 150px;
  height: 115px;
  padding: 20px 0 15px;
` as any

export const StyledTitle = styled(Heading)`
  color: #FFFFFF;
  font-weight: 400;
  line-height: 22px;
  margin: 0;
  display: inline-block;
` as any

export const StyledDescText = styled<{}, 'p'>('p')`
  color: #FFFFFF;
  font-size: 16px;
  font-weight: normal;
  letter-spacing: 0;
  line-height: 24px;
  opacity: 0.5;
  margin: 15px auto 23px;
  text-align: center;
  width: 280px;
  font-family: Muli, sans-serif;
` as any

export const StyledButtonWrapper = styled<{}, 'div'>('div')`
  margin: 0 auto 25px;
  display: inline-block;
` as any

export const StyledFooterText = styled<{}, 'p'>('p')`
  color: #73CBFF;
  font-size: 14px;
  font-weight: 500;
  letter-spacing: -0.19px;
  line-height: 21px;
  margin: 0 auto;
` as any

export const StyledTrademark = styled<{}, 'span'>('span')`
  display: inline-block;
  vertical-align: text-top;
  margin-top: -13px;
  color: #FFF;
  font-size: 9px;
  font-weight: 500;
  opacity: 0.7;
` as any
