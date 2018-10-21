/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { ComponentType } from 'react'
import styled from '../../../../theme'
import Heading from '../../../../components/text/heading'
import Button, { Props as ButtonProps } from '../../../../components/buttonsIndicators/button'
import { DuckDuckGoIcon, TorLockIcon } from '../../../../components/icons'

export const Box = styled<{}, 'section'>('section')`
  box-sizing: border-box;
  -webkit-font-smoothing: antialiased;
  padding: 30px 30px 50px;
  border-radius: 12px;
  border: 1px solid rgba(255,255,255,0.25);
`

export const HeaderBox = styled<{}, 'section'>('section')`
  box-sizing: border-box;
  -webkit-font-smoothing: antialiased;
`

export const Content = styled<{}, 'article'>('article')`
  box-sizing: border-box;
  display: block;
  min-height: 285px;
`

export const DuckDuckGoImage = styled(DuckDuckGoIcon)`
  box-sizing: border-box;
  display: block;
  width: 40px;
  margin-bottom: 20px;
`

export const TorLockImage = styled(TorLockIcon)`
  box-sizing: border-box;
  display: block;
  width: 40px;
  margin-bottom: 20px;
`

export const PrivateImage = styled<{}, 'img'>('img')`
  box-sizing: border-box;
  display: block;
  width: 293px;

  @media screen and (max-width: 1170px) {
    margin: auto;
  }
`

export const TorImage = styled<{}, 'img'>('img')`
  box-sizing: border-box;
  display: block;
  width: 177px;
  margin: 0 0 40px;

  @media screen and (max-width: 1170px) {
    margin: auto;
  }
`

export const SubTitle = styled<{}, 'small'>('small')`
  box-sizing: border-box;
  -webkit-font-smoothing: antialiased;
  display: block;
  color: #7642F5;
  font-size: 14px;
  text-transform: uppercase;
  font-weight: 600;
  margin-bottom: 5px;
`

export const Title = styled(Heading)`
  font-size: 38px;
  letter-spacing: 0.02px;
  font-weight: 500;
  color: #fff;
  margin: 0 0 0px;
`
export const Text = styled<{}, 'p'>('p')`
  font-family: ${p => p.theme.fontFamily.body};
  letter-spacing: 0.19px;
  line-height: 26px;
  font-size: 15px;
  margin: 15px 0;
  color: #fff;
`

export const Separator = styled<{}, 'hr'>('hr')`
  border: 1px solid rgba(255,255,255,0.10);
  height: 0;
  width: 100%;
  margin: 25px 0 0;
`
export const PurpleButton = styled(Button as ComponentType<ButtonProps>)`
  background: #5E35C3;
  font-family: ${p => p.theme.fontFamily.heading};
  font-size: 14px;
  font-weight: 500;
  padding: 14px 20px;
  margin: 25px 25px 0 0;
`
export const Link = styled<{}, 'a'>('a')`
  font-family: ${p => p.theme.fontFamily.heading};
  font-size: 14px;
  color: #814EFF;
  font-style: normal;
  line-height: 1;
  align-self: center;
  margin: 25px 20px 0 0;
  cursor: pointer;
  text-decoration: none;
`

interface FakeButtonProps {
  settings?: boolean
  withToggle?: boolean
}

export const FakeButton = styled<FakeButtonProps, 'a'>('a')`
  display: grid;
  height: 100%;
  grid-template-columns: ${p => p.settings ? 'auto 16px' : 'auto auto'};
  grid-template-rows: auto;
  grid-gap: ${p => p.withToggle ? '10px' : '0'};
  background: #5E35C3;
  font-family: ${p => p.theme.fontFamily.heading};
  font-size: 14px;
  font-weight: 500;
  padding: 14px 20px;
  margin: 25px 25px 0 0;
  color: #fff;
  user-select: none;
  border-radius: 20px;
  min-width: 104px;
  width: fit-content;
  text-decoration: none;
  align-items: center;
  line-height: 1;

  & * {
    line-height: 15px;
  }
`
