// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import { color } from '@brave/leo/tokens/css/variables'
import wavesLightUrl from '../../assets/svg-icons/waves-light.svg'
import wavesDarkUrl from '../../assets/svg-icons/waves-dark.svg'

export const Box = styled.div`
  width: 100%;
  height: 100%;
  max-height: 450px;
  background: ${color.container.background};
  position: relative;
  overflow: hidden;
  display: flex;
  flex-direction: column;
  align-items: center;
  align-self: stretch;
`

export const WavesContainer = styled.div`
  width: 100%;
  height: 148px;
  position: absolute;
  bottom: -50px;
  background: linear-gradient(
    180deg,
    rgba(0, 0, 0, 0) 0%,
    ${color.primary[20]} 100%
  );
`

export const Waves = styled.div`
  width: 100%;
  height: 100%;
  background-image: url(${wavesLightUrl});
  background-repeat: no-repeat;
  background-size: contain;
  background-position: top;
  user-select: none;
  pointer-events: none;

  @media (prefers-color-scheme: dark) {
    background-image: url(${wavesDarkUrl});
  }
`
