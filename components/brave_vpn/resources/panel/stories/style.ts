// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import { PanelWrapper } from '../style'

export const PanelFrame = styled(PanelWrapper)`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  box-shadow: 0px 0px 24px rgba(99, 105, 110, 0.36);
  border-radius: 16px;
  overflow: hidden;
`
