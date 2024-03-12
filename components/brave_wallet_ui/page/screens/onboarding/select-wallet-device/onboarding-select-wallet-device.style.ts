// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import ConnectWalletGraphicLight from './images/connect-wallet-graphic.svg'

export const ConnectWalletContainer = styled.div`
  display: flex;
  flex-direction: column;
  width: 100%;
  height: 221px;

  background-image: url(${ConnectWalletGraphicLight});
  background-size: cover;
`
