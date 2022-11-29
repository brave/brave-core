// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

export const Wrapper = styled.div<{ isTab?: boolean }>`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: column;
  background-color: ${(p) => p.theme.color.background02};
  border-radius: ${(p) => p.isTab ? 12 : 8}px;
  padding: ${(p) => p.isTab ? 8 : 0}px;
  border: 2px solid ${(p) => p.theme.color.divider01};
  width: ${(p) => p.isTab ? 'unset' : '285px'};
  position: ${(p) => p.isTab ? 'fixed' : 'relative'};
  top: ${(p) => p.isTab ? '100px' : 'unset'};
  left: ${(p) => p.isTab ? '32px' : 'unset'};
  overflow: ${(p) => p.isTab ? 'visible' : 'hidden'};
  z-index: ${(p) => p.isTab ? 10 : 'unset'};
`
