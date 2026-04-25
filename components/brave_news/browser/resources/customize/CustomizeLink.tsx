// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import { color } from '@brave/leo/tokens/css/variables'

const CustomizeLink = styled.button`
  all: unset;
  font-weight: 600;
  font-size: 12px;
  line-height: 18px;
  color: ${color.button.background};
  cursor: pointer;

`
export default CustomizeLink
