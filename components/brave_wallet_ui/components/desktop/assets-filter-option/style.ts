// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

interface StyleProps {
  selected?: boolean
}

export const Option = styled.li<Partial<StyleProps>>`
  display: flex;
  align-items: center;
  padding: 10px 0;
  font-family: Poppins;
  font-size: 14px;
  line-height: 20px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.text02};
  font-weight: ${(p) => p.selected ? 600 : 'normal'};
  cursor: pointer;
`
