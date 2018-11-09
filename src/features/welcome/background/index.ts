/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled, { keyframes } from 'styled-components'

interface BackgroundProps {
  background: {
    image: string
    position: string
  }
}

export const topToBottom = keyframes`
  from {
    transform: translateY(-100%);
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
  animation-duration: 3s;
  animation-timing-function: ease-out;
  animation-fill-mode: forwards;
  animation-iteration-count: 1;
  overflow: hidden;
`

export const Background = styled<BackgroundProps, 'div'>('div')`
  box-sizing: border-box;
  background: url('${p => p.background.image}') repeat-x;
  width: 500%;
  height: inherit;
  will-change: transform;
  transform: translateX(${p => p.background.position});
  transition: transform ease-out 3s;
`
