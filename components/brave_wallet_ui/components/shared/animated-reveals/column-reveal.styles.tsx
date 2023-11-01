// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

export const RevealableContentContainer = styled.div<{ hideContent?: boolean }>`
  --scroll-offset: ${(p) => (p?.hideContent ? '-400px' : '0px')};
  width: 100%;
  white-space: nowrap;
  overflow: ${(p) => (p.hideContent ? 'hidden' : 'unset')};
  opacity: ${(p) => (p.hideContent ? 0 : 1)};
  transition: opacity 0.3s ease-in-out;
  transition-delay: 0.3s;
  height: auto;
`

export const RevealableContentColumn = styled.div`
  display: flex;
  flex-direction: column;
  margin-top: var(--scroll-offset);
  transition: margin-top 0.3s ease-in-out;
  white-space: nowrap;
  align-items: center;
`
