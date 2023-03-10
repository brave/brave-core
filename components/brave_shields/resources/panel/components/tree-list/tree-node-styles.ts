// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

export const Tree = styled.div`
  display: grid;
  grid-template-columns: 20px 2fr;
  grid-gap: 5px;
  align-items: flex-start;
  color: ${(p) => p.theme.color.text01};
  font-family: ${(p) => p.theme.fontFamily.heading};
  font-size: 12px;
  font-weight: 600;
  font-weight: normal;
  line-height: 1.2;
  margin-bottom: 12px;

  &:last-child {
    margin-bottom: 0;
  }
`

export const TreeControlBox = styled.span`
  display: flex;
  align-items: center;
  justify-content: center;
  position: relative;
`

export const SVGBox = styled.i`
  position: absolute;
  top: 20px;
  left: 10px;

  path {
    stroke: ${(p) => p.theme.color.text03};
  }
`

export const ExpandToggleButton = styled.button`
  --border: 2px solid transparent;
  background-color: transparent;
  padding: 0;
  margin: 0;
  border: var(--border);
  width: 16px;
  height: 14px;
  display: flex;
  align-items: center;
  cursor: pointer;

  &:focus-visible {
    --border: 2px solid ${(p) => p.theme.color.focusBorder};
  }
`

export const TreeContents = styled.div`
  overflow: hidden; /* to wrap contents */
`
