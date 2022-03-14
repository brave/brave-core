/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

import bitflyerVerificationBg from '../assets/bitflyer_verification_bg.png'

export const root = styled.div`
  background: linear-gradient(125.83deg, #392DD1 0%, #22B8CF 99.09%);
  font-family: var(--brave-font-heading);
  height: 338px;
  overflow: hidden;
  margin-bottom: 12px;

  @media screen and (max-width: 800px) {
    width: auto;
  }
`

export const closeIcon = styled.div`
  width: 12px;
  height: 12px;
  color: #fff;
  float: right;
  margin: 11px 11px 0 0;
`

export const background = styled.div`
  max-width: 320px;
  height: 100px;
  &.bitflyer-verification-promo {
    background: url('/${bitflyerVerificationBg}') center/contain no-repeat;
  }
  border-radius: 4px;
  margin: 0 auto;
`

export const content = styled.div`
  max-width: 100%;
  width: 100%;
  height: 338px;
  padding: 18px 26px 12px 26px;
  border-radius: 4px;
`

export const copy = styled.div`
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
  color: #fff;
  max-width: 320px;
  height: 54px;
  text-align: center;
  margin: 0 auto 24px auto;
`

export const title = styled.span`
  font-weight: 600;
  font-size: 20px;
  max-width: 320px;
  line-height: 26px;
  letter-spacing: 0.15px;
  color: #fff;
  display: block;
  margin: 0 auto 9px auto;
  text-align: center;
`

export const disclaimer = styled.span`
  display: block;
  margin: 10px 0px;
  font-size: 11px;
  font-weight: bold;
  color: var(--brave-palette-grey500)
  font-family: Muli, sans-serif;
`

export const learnMoreButton = styled.div`
  display: flex;
  flex-direction: column;
  justify-content: center;
  align-items: center;
  padding: 6px 18px;
  max-width: 320px;
  height: 30px;
  color: #4C54D2;
  background: #fff;
  border-radius: 48px;
  margin: 0 auto 11px auto;
  font-weight: 600;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
`

export const dismissButton = styled.div`
  max-width: 320px;
  height: 20px;
  font-size: 12px;
  line-height: 20px;
  text-align: center;
  letter-spacing: 0.01em;
  color: #F0F2FF;
  margin: 0 auto 23px auto;
`
