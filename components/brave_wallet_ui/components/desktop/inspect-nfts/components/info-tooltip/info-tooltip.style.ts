// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

export const StyledWrapper = styled.div`
  position: absolute;
  display: flex;
  flex-direction: row;
  align-items: center;
  z-index: 3;
  background-color: #1e2029;
  width: 238px;
  border-radius: 6px;
  top: 25px;
  right: -76px;
`

export const TooltipContent = styled.div`
  position: relative;
  padding: 24px;
  width: 100%;
  height: 100%;
`

export const TooltipText = styled.p`
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 400;
  font-size: 13px;
  line-height: 20px;
  letter-spacing: 0.01em;
  padding: 0;
  margin: 0;
  color: #f6f6fa;
`

export const ArrowUp = styled.div`
  width: 0;
  height: 0;
  border-left: 8px solid transparent;
  border-right: 8px solid transparent;
  border-bottom: 8px solid #1e2029;
  position: absolute;
  right: 74px;
  top: -8px;
`
