// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

export const ErrorTextRow = styled.div<{
  hasError: boolean
}>`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: center;
  height: ${(p) => p.hasError ? 'auto' : '38px'};
  margin-top: 20px;
  padding-top: 10px;
  margin-bottom: 12px;
`
