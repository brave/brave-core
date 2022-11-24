// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'

interface StyleProps {
  floating?: boolean
}

export const StyledSelect = styled('select')<StyleProps>`
  width: 100%;
  background: #fff;
  height: 34px;
  font-size: 14px;
  border: ${p => p.floating ? 'none' : '1px solid #DFDFE8'};
  text-align-last: ${p => p.floating ? 'right' : 'left'};

  &:focus {
    outline: 0;
  }
`
