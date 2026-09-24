// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'

export const Wrapper = styled.div`
  position: relative;
  display: flex;
  align-items: center;
  justify-content: center;
  width: 390px;
  height: 650px;
  background-color: ${leo.color.page.background};
  /* Cr151+ bubble autosize uses document scrollWidth; clip overflow so
     absolute/fixed children cannot inflate the panel beyond its set size. */
  overflow: hidden;
  contain: layout;
`

export const SidePanelWrapper = styled.div`
  display: flex;
  align-items: center;
  justify-content: flex-start;
  flex-direction: column;
  width: 100vw;
  height: 100vh;
  background-color: ${leo.color.page.background};
`
