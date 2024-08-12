// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import { color, font, spacing } from '@brave/leo/tokens/css/variables'
import ProgressRing from '@brave/leo/react/progressRing'
import { layoutPanelWidth } from '../../../components/desktop/wallet-page-wrapper/wallet-page-wrapper.style'
import { Column, ScrollableColumn } from '../../../components/shared/style'

export const ContentWrapper = styled(Column)`
  @media screen and (max-width: ${layoutPanelWidth}px) {
    margin: 8px;
  }
`

export const ControlPanel = styled.div`
  display: grid;
  grid-template-columns: 160px auto auto;
  gap: 16px;
  width: 100%;
  align-items: flex-start;

  @media screen and (max-width: ${layoutPanelWidth}px) {
    grid-template-columns: 1fr;
    border-radius: 16px;
    padding: 8px;
    background-color: ${color.container.background};
  }
`

export const ServiceProvidersWrapper = styled(ScrollableColumn)`
  display: flex;
  flex-wrap: wrap;
  gap: ${spacing['3Xl']};

  @media screen and (max-width: ${layoutPanelWidth}px) {
    border-radius: 16px;
    background-color: ${color.container.background};
  }
`

export const LoadingWrapper = styled(Column)`
  justify-content: center;
  align-items: center;
  width: 100%;
  height: 300px;
`

export const LoaderText = styled.p`
  color: ${color.text.primary};
  font: ${font.default.regular};
  text-align: center;
`

export const Loader = styled(ProgressRing).attrs({
  mode: 'indeterminate'
})`
  --leo-progressring-size: 32px;
`

export const Divider = styled.div`
  width: 100%;
  height: 1px;
  background-color: ${color.divider.subtle};
  margin-top: ${spacing['3Xl']};

  @media screen and (max-width: ${layoutPanelWidth}px) {
    display: none;
  }
`
