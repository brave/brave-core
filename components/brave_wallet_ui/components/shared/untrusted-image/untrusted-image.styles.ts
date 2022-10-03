// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import styled, { css } from 'styled-components'
import { LoaderIcon } from 'brave-ui/components/icons'

type SizeProps = {
  width?: number | string
  height?: number | string
}

export const LoadingOverlay = styled.div<{ isLoading: boolean }>`
  display: ${(p) => p.isLoading ? 'flex' : 'none'};
  flex-direction: column;
  align-items: center;
  justify-content: center;
  width: 100%;
  height: 100%;
  position: absolute;
  z-index: 10;
`

export const LoadIcon = styled(LoaderIcon)`
  color: ${p => p.theme.color.interactive08};
  height: 20px;
  width: 20px;
  opacity: .4;
`

export const UntrustedImageIframe = styled.iframe<
  SizeProps & { responsive?: boolean }
>`
  ${(p) =>
    p?.responsive
      ? css`
          border: none;
          position: absolute;
          z-index: 2;
          top: 0;
          left: 0;
          bottom: 0;
          right: 0;
          width: 100%;
          height: 100%;
        `
      : css<SizeProps>`
        border: none;
        width: ${(p) => p?.width ?? '40px'};
        height: ${(p) => p?.height ?? '40px'};
      `
    }
`
