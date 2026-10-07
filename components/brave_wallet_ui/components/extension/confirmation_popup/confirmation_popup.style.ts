// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'

export const Wrapper = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  left: 0;
  right: 0;
  top: 0;
  bottom: 0;
  position: fixed;
  z-index: 30;
  backdrop-filter: blur(8px);
  padding: 32px;
`

export const Popup = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: flex-start;
  position: relative;
  width: 100%;
  max-width: 500px;
  max-height: 80vh;
  min-height: 60vh;
  border-radius: ${leo.radius.xl};
  box-shadow: 0px 0px 15px rgba(0, 0, 0, 0.25);
  box-sizing: border-box;
  overflow: hidden;
  background-color: ${leo.color.page.background};
  /* Makes position:fixed BottomSheets (Details, Advanced settings, fee
     editors) use this popup as their containing block instead of the
     viewport / full-screen overlay. */
  transform: translateZ(0);
`
