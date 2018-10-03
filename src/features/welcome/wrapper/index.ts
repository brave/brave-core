/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled, { css, keyframes } from 'styled-components'
import { Card } from '../../../index'

const fadeIn = keyframes`
  from {
    opacity: 0;
  }
  to {
    opacity: 1;
  }
`

const slideOut = keyframes`
  from {
    transform: translateX(0);
  }
  to {
    transform: translateX(-150%);
  }
`

const slideIn = keyframes`
  from {
    transform: translateX(150%);
  }
  to {
    transform: translateX(0);
  }
`

const BaseGrid = styled<{}, 'div'>('div')`
  box-sizing: border-box;
  display: grid;
  height: 100%;
`

const BaseColumn = styled<{}, 'div'>('div')`
  box-sizing: border-box;
  position: relative;
  display: flex;
`

export const SelectGrid = styled(BaseGrid)`
  display: grid;
  height: 100%;
  grid-template-columns: 2fr 1fr;
  grid-template-rows: 1fr;
  padding: 0 30px;
  grid-gap: 30px;
  align-items: center;
`

export const Footer = styled(BaseGrid.withComponent('footer'))`
  grid-template-columns: 1fr 1fr 1fr;
  grid-template-rows: 1fr;
  padding: 0 60px 50px;
`

export const FooterLeftColumn = styled(BaseColumn)`
  align-items: center;
  justify-content: flex-start;
`

export const FooterMiddleColumn = styled(BaseColumn)`
  align-items: center;
  justify-content: center;
`

export const FooterRightColumn = styled(BaseColumn)`
  align-items: center;
  justify-content: flex-end;
`
interface ContentProps {
  active: boolean
  zIndex: number
}

export const Content = styled<ContentProps, 'section'>('section')`
  /* animation start state must be the same as "from" keyframe */
  transform: translateX(0);
  /* animation stuff courtesy of ross */
  animation-delay: 0;
  animation-name: ${slideOut};
  animation-duration: 1.5s;
  animation-timing-function: ease-in-out;
  animation-fill-mode: forwards;
  /* end of animation stuff */
  position: absolute;
  z-index: ${p => p.zIndex};
  display: flex;
  flex-direction: column;
  justify-content: center;
  align-items: center;
  flex: 1;
  padding: 0 60px;

  ${p => p.active && css`
    /* animation start state must be the same as "from" keyframe */
    transform: translateX(150%);
    /* animation stuff courtesy of ross */
    animation-delay: 0;
    animation-name: ${slideIn};
    animation-duration: 1.5s;
    animation-timing-function: ease-in-out;
    animation-fill-mode: forwards;
    /* end of animation stuff */
  `}
`

export const Page = styled<{}, 'div'>('div')`
  position: fixed;
  top: 0;
  left: 0;
  width: 100%;
  height: 100%;
  display: flex;
  align-items: center;
  justify-content: center;
`

export const Panel = styled(Card)`
  /* animation start state must be the same as "from" keyframe */
  opacity: 0;
  /* animation stuff courtesy of ross */
  animation-delay: .5s;
  animation-name: ${fadeIn};
  animation-duration: 1s;
  animation-timing-function: ease-in-out;
  animation-fill-mode: forwards;
  /* end of animation stuff */
  position: relative;
  overflow: hidden;
  background-color: rgba(255,255,255,0.99);
  border-radius: 20px;
  box-shadow: 0 6px 12px 0 rgba(39, 46, 64, 0.2);
  max-width: 600px;
  min-height: 580px;
  display: flex;
  flex-direction: column;
  justify-content: space-between;
  padding: 0;
`
