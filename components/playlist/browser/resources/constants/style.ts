// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { css } from 'styled-components'

const miniPlayerHeight = 80
const playlistControlsAreaHeights = 64

export const playerTypes = {
  miniPlayer: `(max-height: ${miniPlayerHeight + 'px'})`,
  normalPlayer: `(min-height: ${miniPlayerHeight + 1 + 'px'})`
}

export const hiddenOnMiniPlayer = css`
  @media ${playerTypes.miniPlayer} {
    display: none;
  }
`

export const hiddenOnNormalPlayer = css`
  @media ${playerTypes.normalPlayer} {
    display: none;
  }
`

export const playerVariables = css`
  --player-controls-area-height: ${playlistControlsAreaHeights + 'px'};
  --mini-player-height: ${miniPlayerHeight + 'px'};
`
