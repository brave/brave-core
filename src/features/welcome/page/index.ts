/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'
import { Card } from '../../../index'

interface WaveBackgroundProps {
  background: {
    image: string
    position: string
  }
}

export const WaveBackground = styled<WaveBackgroundProps, 'div'>('div')`
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

export const WelcomePanel = styled(Card)`
  background-color: rgba(255,255,255,0.95);
  border-radius: 20px;
  box-shadow: 0 6px 12px 0 rgba(39, 46, 64, 0.2);
  max-width: 600px;
  min-height: 580px;
  display: flex;
  flex-direction: column;
  justify-content: space-between;
  padding: 50px 60px
`
