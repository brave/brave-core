// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

export const StyledWrapper = styled.div`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: space-between;
  width: 100%;
  border: ${(p) => `1px solid ${p.theme.color.interactive08}`};
  border-radius: 4px;
`

export const HorizontalDivider = styled.div`
  display: flex;
  width: 1px;
  height: 24px;
  background-color: ${(p) => p.theme.color.interactive08};
`
