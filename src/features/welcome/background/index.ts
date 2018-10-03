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

const topToBottom = keyframes`
  from {
    transform: translateY(-100%);
  }

  to {
    transform: translateY(0);
  }
`

export const Background = styled<BackgroundProps, 'div'>('div')`
  /* animation start state must be the same as "from" keyframe */
  transform: translateY(-100%);
  /* animation stuff courtesy of ross */
  animation-delay: 0s;
  animation-name: ${topToBottom};
  animation-duration: 3s;
  animation-timing-function: ease-out;
  animation-fill-mode: forwards;
  /* end of animation stuff */
  box-sizing: border-box;
  font-family: ${p => p.theme.fontFamily.heading};
  width: inherit;
  height: inherit;
  background-repeat: repeat-x;
  background-image: url('${p => p.background.image}');
  background-position-x: ${p => p.background.position};
  transition: 2.5s ease-in-out;
`
