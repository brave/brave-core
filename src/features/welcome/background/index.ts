/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

interface BackgroundProps {
  background: {
    image: string
    position: string
  }
}

export const Background = styled<BackgroundProps, 'div'>('div')`
  box-sizing: border-box;
  display: flex;
  align-items: center;
  justify-content: center;
  font-family: ${p => p.theme.fontFamily.heading};
  height: 100%;
  width: 100%;
  background-repeat: repeat-x;
  background-image: url('${p => p.background.image}');
  background-position-x: ${p => p.background.position};
  transition: 1.5s ease-in-out;
`
