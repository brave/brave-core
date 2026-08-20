// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'
import Alert from '@brave/leo/react/alert'

// Shared Styles
import { Column, Row } from '../../../components/shared/style'
import {
  layoutPanelWidth, //
} from '../../../components/desktop/wallet-page-wrapper/wallet-page-wrapper.style'

export const ControlsWrapper = styled(Column)`
  padding: 0px 32px;
  @media screen and (max-width: ${layoutPanelWidth}px) {
    padding: 16px 16px 0px 16px;
  }
`

export const AssetsWrapper = styled(Column)`
  padding: 16px 20px 20px 20px;
  @media screen and (max-width: ${layoutPanelWidth}px) {
    padding: 8px;
  }
`

export const NFTsWrapper = styled(Column)`
  padding: 16px;
  @media screen and (max-width: ${layoutPanelWidth}px) {
    padding: 8px;
  }
`

export const TransactionsWrapper = styled(Column)`
  padding: 16px 24px 24px 24px;
  @media screen and (max-width: ${layoutPanelWidth}px) {
    padding: 16px;
  }
`

export const EmptyStateWrapper = styled(Column)`
  padding: 32px;
  @media screen and (max-width: ${layoutPanelWidth}px) {
    padding: 16px;
  }
`

export const SyncAlertWrapper = styled(Row)`
  padding: 0px 32px;
  margin: 0px 0px 32px 0px;
  @media screen and (max-width: ${layoutPanelWidth}px) {
    padding: 16px 16px 0px 16px;
    margin: 0px;
  }
`

export const SyncAlert = styled(Alert)`
  width: 100%;
`
