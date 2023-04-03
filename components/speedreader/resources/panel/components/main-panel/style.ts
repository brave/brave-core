// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'

export const Box = styled.div`
  display: flex;
  background: ${(p) => p.theme.color.background01};
  color: ${(p) => p.theme.color.text01};
  width: 870px;
  height: 40px;
  font-family: ${(p) => p.theme.fontFamily.heading};
  user-select: none;
  margin: 0 auto;
`
