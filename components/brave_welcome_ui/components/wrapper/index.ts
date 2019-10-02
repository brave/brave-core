/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled, { css, keyframes } from 'styled-components'

const fadeIn = keyframes`
  from {
    opacity: 0;
  }
  to {
    opacity: 1;
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
  display: flex;
  height: 100%;
  padding: 0 30px;
  align-items: center;
  width: 80%;
`

export const Footer = styled(BaseGrid.withComponent('footer'))`
  grid-template-columns: 1fr 1fr 1fr;
  grid-template-rows: 52px;
  max-width: 540px;
  margin: 24px 0 0 0;
`

export const FooterLeftColumn = styled(BaseColumn)`
  align-items: center;
  justify-content: center;
`

export const FooterMiddleColumn = styled(BaseColumn)`
  align-items: center;
  justify-content: center;
`

export const FooterRightColumn = styled(BaseColumn)`
  align-items: center;
  justify-content: center;
`

interface ContentProps {
  active: boolean
  zIndex: number
  screenPosition: string
  isPrevious: boolean
}

export const Content = styled<ContentProps, 'section'>('section')`
  opacity: 0;
  will-change: transform;
  transform: translateX(${p => p.isPrevious ? '-' + p.screenPosition : p.screenPosition}) scale(0.8);
  transition: opacity 600ms, transform 600ms ease-in-out;
  position: absolute;
  z-index: ${p => p.zIndex};
  display: flex;
  flex-direction: column;
  justify-content: center;
  align-items: center;
  flex: 1;
  max-width: 580px;
  padding: 24px;

  ${p => p.active && css`
    opacity: 1;
    transform: translateX(0) scale(1);
  `}
`

export const Page = styled<{}, 'div'>('div')`
  position: absolute;
  width: inherit;
  height: inherit;
  display: flex;
  align-items: flex-start;
  justify-content: center;
  background: ${p => p.theme.color.panelBackground};
  overflow: hidden;
  transition: background 0.3s linear;
`

export const Panel = styled('div')`
  user-select: none;
  /* animation start state must be the same as "from" keyframe */
  opacity: 0;
  /* animation stuff courtesy of ross */
  animation-delay: 1s;
  animation-name: ${fadeIn};
  animation-duration: 1200ms;
  animation-timing-function: ease-out;
  animation-fill-mode: forwards;
  /* end of animation stuff */
  box-sizing: border-box;
  position: relative;
  overflow: visible;
  max-width: 800px;
  width: 100%;
  display: flex;
  flex-direction: column;
  padding: 0;
  margin: 0;
  font-size: inherit;
  align-items: center;
`

export const SlideContent = styled<{}, 'div'>('div')`
  max-width: inherit;
  width: inherit;
  min-height: 540px;
  display: flex;
  justify-content: center;
  align-items: center;
`
