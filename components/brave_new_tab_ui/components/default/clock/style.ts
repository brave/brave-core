// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import styled from 'brave-ui/theme'

export const StyledClock = styled<{}, 'div'>('div')`
  color: #FFFFFF;
  box-sizing: border-box;
  line-height: 1;
  user-select: none;
  display: flex;
  -webkit-font-smoothing: antialiased;
  font-family: ${p => p.theme.fontFamily.heading};
`

export const StyledTime = styled<{}, 'span'>('span')`
  box-sizing: border-box;
  font-size: 78px;
  font-weight: 300;
  color: inherit;
  display: inline-flex;
`

export const StyledTimeSeparator = styled<{}, 'span'>('span')`
  box-sizing: border-box;
  color: inherit;
  font-size: inherit;
  font-weight: 200;
  /* center colon vertically in the text-content line */
  margin-top: -0.1em;
`
