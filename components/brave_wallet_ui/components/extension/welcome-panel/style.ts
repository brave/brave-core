// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'

export const StyledWrapper = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  background-color: ${(p) => p.theme.color.background01};
  padding: 24px;
`

export const Title = styled.span`
  font: ${leo.font.heading.h3};
color: ${(p) => p.theme.color.text01};
  margin-bottom: 12px;
`

export const Description = styled.span`
  font: ${leo.font.default.regular};
color: ${(p) => p.theme.color.text02};
  max-width: 270px;
  text-align: center;
  margin-bottom: 35px;
`
