// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'

// Status indicator
export const StatusBox = styled.div`
  display: flex;
  align-items: center;
  margin: 20px 0 24px 0;
`

export const StatusText = styled.span`
  font-family: Poppins;
  font-size: 14px;
  font-weight: 400;
  line-height: 20px;
  color: ${(p) => p.theme.color.text01};
  letter-spacing: 0.04em;
`

export const ActiveIndicator = styled.span`
  width: 6px;
  height: 6px;
  margin-right: 8px;
  border-radius: 50%;
  background: ${(p) => p.theme.color.successBorder};
`

export const InActiveIndicator = styled(ActiveIndicator)`
  background: ${(p) => p.theme.color.disabled};
`

export const FailedIndicator = styled(ActiveIndicator)`
  background: ${(p) => p.theme.color.warn};
`

export const Loader = styled.span`
  width: 12px;
  height: 12px;
  margin-right: 8px;

  svg {
    fill: ${(p) => p.theme.color.text03};
  }
`
