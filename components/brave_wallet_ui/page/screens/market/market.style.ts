// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as leo from '@brave/leo/tokens/css/variables'
import styled from 'styled-components'
import { LoaderIcon } from 'brave-ui/components/icons'

export interface StyleProps {
  alignment: 'right' | 'left'
}

export const TopRow = styled.div`
  display: flex;
  flex-direction: row;
  align-items: flex-start;
  justify-content: flex-start;
  width: 100%;
  gap: 10px;
  position: static;
  top: 0;
`

export const AssetsColumnWrapper = styled.div`
  display: inline-flex;
  flex-direction: row;
  align-items: center;
  justify-content: left;
`

export const AssetsColumnItemSpacer = styled.div`
  display: inline-flex;
  align-items: center;
  justify-content: center;
  margin-right: 19px;
`
export const TextWrapper = styled.div<StyleProps>`
  font: ${leo.font.default.regular};

  display: flex;
  justify-content: ${(p) =>
    p.alignment === 'right' ? 'flex-end' : 'flex-start'};
  width: 100%;
  letter-spacing: 0.01em;
`

export const LineChartWrapper = styled.div`
  display: flex;
  align-items: center;
  justify-content: flex-end;
  height: 30px;
  max-width: 120px;
  margin: 0 auto;
`

export const LoadIcon = styled(LoaderIcon)`
  color: ${leo.color.neutral[30]};
  height: 70px;
  width: 70px;
  opacity: 0.4;
`

export const LoadIconWrapper = styled.div`
  display: flex;
  width: 100%;
  height: 100%;
  flex-direction: column;
  align-items: center;
  justify-content: center;
`
export const MarketDataIframe = styled.iframe<{
  iframeHeight: number
}>`
  width: 100%;
  height: 100%;
  min-height: ${(p) =>
    p.iframeHeight ? `calc(${p.iframeHeight}px + 72px)` : 'unset'};
  border: none;
`

export const StyledWrapper = styled.div`
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  justify-content: flex-start;
  width: 100%;
  height: 100%;
`

export const ButtonRow = styled.div<{
  noMargin?: boolean
  horizontalPadding?: number
}>`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: flex-start;
  width: 100%;
  margin: ${(p) => (p.noMargin ? '0px' : '20px 0px')};
  padding: 0px
    ${(p) => (p.horizontalPadding !== undefined ? p.horizontalPadding : 0)}px;
  gap: 12px;
`
