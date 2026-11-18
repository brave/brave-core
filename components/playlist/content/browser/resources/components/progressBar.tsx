/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

import LeoProgressBar from '@brave/nala/react/progressBar'
import { color } from '@brave/nala/tokens/css/variables'

export const ProgressBar = styled(LeoProgressBar)`
  position: absolute;
  bottom: 0;
  width: 100%;
  height: 3px;
  --leo-progressbar-radius: 0;
  --leo-progressbar-background-color: color-mix(
    in srgb,
    ${color.white} 40%,
    transparent
  );
`
