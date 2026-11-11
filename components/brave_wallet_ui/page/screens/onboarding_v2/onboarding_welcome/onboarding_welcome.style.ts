// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/nala/tokens/css/variables'

// Shared Styles
import { Column, Text } from '$wallet/components/shared/style'
import { layoutPanelWidth } from '$wallet/page/components/wallet_page_wrapper/wallet_page_wrapper.style'

export const WelcomePageWrapper = styled.div`
  display: flex;
  justify-content: center;
  align-items: center;
  width: 100%;
  height: 100vh;
`

export const Content = styled(Column)`
  display: flex;
  align-items: center;
  justify-content: flex-start;
  z-index: 3;
  max-width: 812px;

  @media screen and (min-width: 768px) and (max-width: 1024px) {
    width: 100%;
  }
`

export const Title = styled(Text)`
  max-width: 280px;
`

export const Description = styled(Text)`
  max-width: 640px;
`

export const ActionsContainer = styled.div`
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  width: 100%;
  gap: ${leo.spacing.xl};
  padding-bottom: ${leo.spacing['3Xl']};
  @media (min-width: ${layoutPanelWidth}px) {
    flex-direction: row;
  }
`
