// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import { LoaderIcon } from 'brave-ui/components/icons'
import { StyleProps } from '../nft-details/nft-details-styles'

interface Props {
  customWidth?: string
  customHeight?: string
}

export const Image = styled.img<Props>`
  border-radius: 4px;
  width: 100%;
  height: auto
`

export const LoadingOverlay = styled.div<Partial<StyleProps>>`
  display: ${(p) => p.isLoading ? 'flex' : 'none'};
  flex-direction: column;
  align-items: center;
  justify-content: center;
  width: 100%;
  height: 440px;
  position: absolute;
  z-index: 10;
  backdrop-filter: blur(5px);
`

export const LoadIcon = styled(LoaderIcon)`
  color: ${p => p.theme.color.interactive08};
  height: 70px;
  width: 70px;
  opacity: .4;
`
