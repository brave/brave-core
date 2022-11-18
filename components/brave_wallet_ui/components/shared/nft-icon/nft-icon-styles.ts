// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

export const NftImageIframe = styled.iframe<{ circular?: boolean }>`
  border: none;
  width: ${p => p.width ? p.width : '40px'};
  height: ${p => p.height ? p.height : '40px'};
  border-radius: ${p => p.circular ? '50%' : 0};
`

export const NftImageResponsiveIframe = styled.iframe<{ circular?: boolean }>`
  border: none;
  position: absolute;
  z-index: 2;
  top: 0;
  left: 0;
  bottom: 0;
  right: 0;
  width: 100%;
  height: 100%;
  border-radius: ${p => p.circular ? '50%' : 0};
`

export const NftIconWrapper = styled.div`
  position: relative;
  display: flex;
  align-items: center;
  justify-content: center;
`

export const IconWrapper = styled.div<{ disabled?: boolean }>`
  position: absolute;
  width: 24px;
  height: 24px;
  z-index: 3;
  bottom: 0px;
  right: 12px;
  filter: ${(p) => p.disabled ? 'grayscale(100%)' : 'none'};
`
