/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled, { keyframes } from 'styled-components'

import LionImage from './lion_logo.svg'
import ImportImage from './welcome_import.svg'
import RewardsImage from './welcome_rewards.svg'
import SearchImage from './welcome_search.svg'
import ShieldsImage from './welcome_shields.svg'
import WelcomeImage from './welcome_bg.svg'

const BaseImage = styled<{}, 'img'>('img')`
  box-sizing: border-box;
  display: block;
  max-width: 100%;
`

export const WelcomeLionImage = styled(BaseImage).attrs({ src: LionImage })`
  height: 140px;
`

export const WelcomeImportImage = styled(BaseImage).attrs({ src: ImportImage })`
  height: 190px;
`

export const WelcomeRewardsImage = styled(BaseImage).attrs({ src: RewardsImage })`
  height: 190px;
`

export const WelcomeSearchImage = styled(BaseImage).attrs({ src: SearchImage })`
  height: 190px;
`

export const WelcomeShieldsImage = styled(BaseImage).attrs({ src: ShieldsImage })`
  height: 140px;
`

export const topToBottom = keyframes`
  from {
    transform: translateY(100%);
  }

  to {
    transform: translateY(0);
  }
`

export const BackgroundContainer = styled<{}, 'div'>('div')`
  box-sizing: border-box;
  width: inherit;
  height: inherit;
  position: absolute;
  animation-delay: 0s;
  animation-name: ${topToBottom};
  animation-duration: 2000ms;
  animation-timing-function: ease-in-out;
  animation-fill-mode: forwards;
  animation-iteration-count: 1;
  overflow: hidden;
`

export const Background = styled<{}, 'div'>('div')`
  box-sizing: border-box;
  background: url('${WelcomeImage}');
  width: 100%;
  height: 136px;
  background-size: cover;
  background-position-x: center;
  position: absolute;
  bottom: 0;
  overflow: hidden;
`
