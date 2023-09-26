// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css'
import Ring from '@brave/leo/react/progressRing'
import { layoutPanelWidth } from '../../../../wallet-page-wrapper/wallet-page-wrapper.style'
import { WalletButton } from '../../../../../shared/style'

export const StyledWrapper = styled.div`
  display: flex;
  flex-direction: column;
  width: 100%;
`

export const CategoryWrapper = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  margin-top: ${leo.spacing['2xl']};

  @media screen and (max-width: ${layoutPanelWidth}px) {
    margin-top: ${leo.spacing.m};
  }
`

export const DappGrid = styled.div`
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  column-gap: 32px;
  width: 100%;

  @media screen and (max-width: ${layoutPanelWidth}px) {
    grid-template-columns: repeat(1, minmax(0, 1fr));
  }
`
export const CategoryTitle = styled.span`
  display: flex;
  align-self: flex-start;
  padding-left: ${leo.spacing.m};
  color: ${leo.color.text.primary};
  font-family: Poppins;
  font-size: 16px;
  font-style: normal;
  font-weight: 500;
  line-height: 28px;
  text-align: left;
  text-transform: capitalize;
`

export const ListToggleButton = styled(WalletButton)`
  display: flex;
  padding: 10px 0px;
  justify-content: center;
  align-items: center;
  align-self: stretch;
  color: ${leo.color.text.interactive};
  font-family: Poppins;
  font-size: 12px;
  font-style: normal;
  font-weight: 600;
  line-height: 16px;
  letter-spacing: 0.36px;
  background-color: transparent;
  outline: none;
  border: none;
  cursor: pointer;
`

export const LoadingRing = styled(Ring)`
  --leo-progressring-size: 64px;
`
