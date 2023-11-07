// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled, { css } from 'styled-components'
import { LoaderIcon } from 'brave-ui/components/icons'

export const StyledWrapper = styled.div<{
  customStyle?: { [key: string]: string }
}>`
  width: 100%;
  height: 130px;
  min-height: 130px;
  max-height: 130px;
  box-sizing: border-box;
  position: relative;
  ${(p) =>
    p.customStyle
      ? css`
          ${p.customStyle}
        `
      : ''};
`

export const AreaWrapper = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  width: 100%;
  height: 100%;
  position: absolute;
`

export const LoadingOverlay = styled.div<{
  isLoading: boolean
}>`
  display: ${(p) => (p.isLoading ? 'flex' : 'none')};
  flex-direction: column;
  align-items: center;
  justify-content: center;
  width: 100%;
  height: 100%;
  position: absolute;
  backdrop-filter: blur(5px);
`

export const LoadIcon = styled(LoaderIcon)`
  color: ${(p) => p.theme.color.interactive08};
  height: 70px;
  width: 70px;
  opacity: 0.4;
`
