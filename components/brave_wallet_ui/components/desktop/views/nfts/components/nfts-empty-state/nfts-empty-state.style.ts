// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as leo from '@brave/leo/tokens/css/variables'
import styled from 'styled-components'

import { WalletButton } from '../../../../../shared/style'

export const StyledWrapper = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  width: 100%;
`

export const EmptyStateImage = styled.img`
  width: 241px;
  height: auto;
  margin-bottom: 16px;
`

export const Heading = styled.h2`
  font: ${leo.font.large.semibold};
  text-align: center;
  color: ${leo.color.text.primary};
  margin: 0 0 8px 0;
`

export const SubHeading = styled.p`
  font: ${leo.font.default.regular};
  align-items: center;
  text-align: center;
  color: ${leo.color.text.secondary};
  margin: 0 0 16px 0;
`

export const ImportButton = styled(WalletButton)`
  font: ${leo.font.components.tableheader};

  display: flex;
  flex-direction: column;
  justify-content: center;
  align-items: center;
  padding: 10px 22px;
  gap: 7px;
  background-color: ${leo.color.button.background};
  border-radius: 48px;
  text-align: center;
  color: ${leo.color.page.background};
  margin-bottom: 24px;
  border: none;
  cursor: pointer;
`

export const DisclaimerText = styled.p`
  font: ${leo.font.xSmall.regular};
  display: flex;
  align-items: center;
  text-align: center;
  color: ${leo.color.text.tertiary};
  margin: 0;
`
