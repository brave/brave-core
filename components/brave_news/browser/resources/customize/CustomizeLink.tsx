// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import { color, font } from '@brave/leo/tokens/css/variables'

const CustomizeLink = styled.button`
  all: unset;
  font: ${font.small.semibold};
  color: ${color.text.interactive};
  cursor: pointer;
`
export default CustomizeLink
