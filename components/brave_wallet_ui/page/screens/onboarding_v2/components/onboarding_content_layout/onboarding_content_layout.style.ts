// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/nala/tokens/css/variables'
import Button from '@brave/nala/react/button'

export const StyledWrapper = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: flex-start;
  width: 100%;
  min-height: 100vh;
  box-sizing: border-box;
  overflow-x: hidden;
  position: relative;

  > :first-child {
    flex-shrink: 0;
  }
`

export const ContentWrapper = styled.div`
  display: flex;
  position: relative;
  flex-direction: column;
  justify-content: flex-start;
  align-items: flex-start;
  border-radius: ${leo.spacing['2Xl']};
  background: ${leo.color.container.background};
  box-shadow: ${leo.effect.elevation['03']};
  width: 100%;
  max-width: 600px;
  z-index: 2;
  padding: ${leo.spacing['3Xl']};
  flex-shrink: 0;
  margin-top: auto;
  margin-bottom: auto;
`

export const BackButton = styled(Button)`
  --leo-button-color: ${leo.color.text.secondary};
`
