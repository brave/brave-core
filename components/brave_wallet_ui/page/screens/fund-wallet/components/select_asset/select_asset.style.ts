// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import { font, color } from '@brave/leo/tokens/css/variables'
import ProgressRing from '@brave/leo/react/progressRing'

// Shared Styles
import { Row } from '../../../../../components/shared/style'

export const Wrapper = styled.div`
  position: fixed;
  top: 0;
  left: 0;
  width: 100%;
  z-index: 999;
  background-color: white;
  box-shadow: 0 2px 4px rgba(0, 0, 0, 0.1);
`

export const AssetImage = styled.img`
  width: 32px;
  height: 32px;
  border-radius: 50%;
`
export const AssetSymbol = styled.span`
  display: flex;
  align-items: center;
  justify-content: center;
  width: 32px;
  height: 32px;
  border-radius: 50%;
  font: ${font.default.semibold};
  color: ${color.container.background};
  background-color: ${color.primary[50]};
`

export const AssetName = styled.span`
  color: ${color.text.primary};
  font: ${font.default.semibold};
  text-overflow: ellipsis;
  text-align: left;
  white-space: nowrap;
  overflow: hidden;
`

export const AssetNetwork = styled.span`
  color: ${color.text.secondary};
  font: ${font.small.regular};
  white-space: nowrap;
  text-align: left;
  overflow: hidden;
  text-overflow: ellipsis;
`

export const AssetPrice = styled.span`
  display: flex;
  color: ${color.text.primary};
  font: ${font.default.regular};
  text-transform: uppercase;
`

export const Loader = styled(ProgressRing)`
  --leo-progressring-size: 22px;
`

export const AutoSizerStyle = {
  width: '100%',
  flex: 1
}

export const SearchAndNetworkFilterRow = styled(Row)`
  background-color: ${color.container.highlight};
  border-radius: 8px;
`
