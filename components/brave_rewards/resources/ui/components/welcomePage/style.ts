/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'
import Heading from '../../../components/text/heading'
import centerBackgroundUrl from './assets/centerTextBackground.svg'

export const StyledOptInSection = styled<{}, 'section'>('section')`
  margin: 40px auto;
  max-width: 303px;

  @media (max-width: 410px) {
    margin: 40px 20px;
    max-width: unset;
  }
`

export const StyledOptInSecond = styled<{}, 'section'>('section')`
  max-width: 303px;
  margin: 0 auto;
`

export const StyledOptInInnerSection = styled<{}, 'section'>('section')`
  text-align: center;
`

export const StyledCenterSection = styled<{}, 'section'>('section')`
  padding: 15px 0 0;
`

export const StyledCenterContent = styled<{}, 'div'>('div')`
  margin: 0 auto;
  max-width: 692px;
  padding: 67px 0 20px;

  @media (max-width: 767px) {
    max-width: none;
    width: 100%;
    padding-top: 30px;
  }
`

export const StyledSection = styled<{}, 'div'>('div')`
  display: block;
`

export const StyledCenterInner = styled<{}, 'section'>('section')`
  @media (max-width: 767px) {
    margin: 0 auto;
    width: 80%;
  }
`

export const StyledInfoContent = styled<{}, 'section'>('section')`
  margin: 22px auto 0;
  max-width: 900px;
`

export const StyledTakeActionContent = styled<{}, 'section'>('section')`
  margin: 0 auto;
  padding: 64px 0 79px;
  max-width: 500px;
  display: block;
`

export const StyledBackground = styled<{}, 'div'>('div')`
  background: url(${centerBackgroundUrl}) no-repeat top;

  @media (max-width: 980px) {
    background: #F8FAFF;
  }
`

export const StyledBatLogo = styled<{}, 'div'>('div')`
  margin: 5px auto 0;
  height: 152px;

  @media (max-width: 460px) {
    height: 100px;
  }
`

export const StyledRewardsTitle = styled(Heading)`
  font-weight: 500;
  color: #FFF;
  display: inline-block;
  margin: 17px 0 4px;

  @media (max-width: 360px) {
    font-size: 36px;
  }
`

export const StyledActionTitle = styled(Heading)`
  color: #5C58C2;
  font-weight: normal;
  line-height: 28px;
  margin: 18px 0 30px;
`

export const StyledCenterTitle = styled(Heading)`
  color: #222326;
  font-weight: normal;
  text-align: left;
  line-height: 28px;
  padding: 7px 0 21px;
  margin: 0;
`

export const StyledSubTitle = styled(Heading)`
  color: #5BC4FE;
  font-weight: 500;
  text-align: center;
  margin: 18px 0 7px;

  @media (max-width: 360px) {
    font-size: 22px;
  }
`

export const StyledTrademark = styled<{}, 'span'>('span')`
  display: inline-block;
  vertical-align: text-top;
  margin-top: -25px;
  color: #FFF;
  font-size: 14px;
  font-weight: 300;
  opacity: 0.7;
`

export const StyledRewardsParagraph = styled<{}, 'p'>('p')`
  font-size: 16px;
  max-width: 375px;
  margin: 0 auto;
  line-height: 28px;
  color: #FFF;
`

export const StyledTeaserParagraph = styled<{}, 'p'>('p')`
  font-size: 16px;
  margin: 0 0 5px;
  line-height: 28px;
  color: #FFF;
  opacity: 0.5;
`

export const StyledCenterParagraph = styled<{}, 'p'>('p')`
  font-size: 16px;
  line-height: 28px;
  color: #686978;
  letter-spacing: 0.16px;
  max-width: 475px;
  margin: 0 0 15px;
  font-weight: 400;
  font-family: Muli,sans-serif;
`

export const StyledAnchor = styled<{}, 'button'>('button')`
  padding: 0;
  border: none;
  background: none;
  cursor: pointer;
  color: #bf9fe3;
  height: 30px;
  overflow: hidden;
  outline: none;

  &:focus {
    outline: 0;
  }
`

export const StyledHeroInfo = styled<{}, 'div'>('div')`
  @media (max-width: 400px) {
    padding: 0 10px;
  }
`
export const StyledAlert = styled<{}, 'div'>('div')`
  margin-bottom: 30px;
  border: 1px solid #F43405;
  border-radius: 4px;
  overflow: hidden;

   @media (max-width: 767px) {
    display: none;
  }
`

export const StyledAlertContent = styled<{}, 'div'>('div')`
  display: flex;
  width: 100%;
`

export const StyledAlertLeft = styled<{}, 'div'>('div')`
  flex-basis: 70%;
  flex-grow: 1;
`
