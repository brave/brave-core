// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

export const Wrapper = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: column;
  background-color: var(--nav-background);
  border-radius: 12px;
  border: 1px solid var(--nav-border);
  position: fixed;
  top: 100px;
  left: 32px;
  overflow: visible;
  z-index: 10;
  padding: 0px 8px;
`

export const Section = styled.div<{ showBorder?: boolean }>`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: column;
  padding: 8px 0px;
  border-bottom: ${(p) => p.showBorder
    ? `1px solid var(--nav-border)`
    : 'none'};
`
