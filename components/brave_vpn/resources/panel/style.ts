// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled, { css } from 'styled-components'

export const PanelWrapper = styled.div<{width? : string}>`
  ${(p) =>
    p.width
      ? css`
          width: ${p.width};
        `
      : css`
          width: 320px;
        `}
  height: auto;
`
