// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

export const PlaceholderText = styled.p<{
  isBig: boolean
}>`
  font-family: Poppins;
  font-size: ${(p) => p.isBig ? '32px' : '14px'};
  line-height: ${(p) => p.isBig ? '32px' : '20px'};
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.text01};
  height: ${(p) => p.isBig ? '32px' : 'auto'};
  margin-bottom: ${(p) => p.isBig ? '20px' : '0px'};
`
