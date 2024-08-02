// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

// shared styles
import { WalletButton } from '../../../shared/style'
import { TabRow } from '../../shared-panel-styles'

export const QueueStepRow = styled.div`
  display: flex;
  align-items: center;
  justify-content: flex-end;
  flex-direction: row;
`

export const QueueStepText = styled.span`
  font-family: Poppins;
  font-size: 13px;
  color: ${(p) => p.theme.color.text02};
  font-weight: 600;
  margin-right: 9px;
`

export const queueStepButtonRowPadding = '16px 0px 4px 0px'

export const QueueStepButton = styled(WalletButton)<{ needsMargin?: boolean }>`
  font-family: Poppins;
  font-style: normal;
  font-weight: 600;
  font-size: 13px;
  color: ${(p) => p.theme.color.interactive05};
  background: none;
  cursor: pointer;
  outline: none;
  border: none;
  margin: 0;
  padding: 0;
  margin-bottom: ${(p) => (p.needsMargin ? '12px' : '0px')};
`

export const FavIcon = styled.img<{ height?: string }>`
  width: auto;
  height: ${(p) => p?.height || '40px'};
  border-radius: 5px;
  background-color: ${(p) => p.theme.color.background01};
  margin-bottom: 7px;
`

export const NetworkFeeRow = styled(TabRow)`
  margin-top: 8px;
`
