// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

export const Box = styled.div`
  display: flex;
  justify-content: flex-end;
  align-items: center;
  width: 100%;
  gap: 16px;
`

export const VDelemiter = styled.div`
  width: 1px;
  height: 20px;
  background: var(--color-border);
`

export const Close = styled.div`
  background: transparent;
  cursor: pointer;
`