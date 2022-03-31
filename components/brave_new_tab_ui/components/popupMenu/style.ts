// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import styled, { css } from 'styled-components'

export const Menu = styled('ul')`
  z-index: 1;
  list-style: none;
  list-style-type: none;
  margin: 0;
  position: absolute;
  width: max-content;
  min-width: 166px;
  top: 114%;
  right: 0;
  border-radius: 4px;
  box-shadow: 0px 0px 6px 0px rgba(0, 0, 0, 0.3);
  padding: 8px 0;
  background-color: ${p => p.theme.color.contextMenuBackground};
  color:  ${p => p.theme.color.contextMenuForeground};
`

export const MenuItem = styled('li')`
  list-style-type: none;
  padding: 10px 18px;
  outline: none;
  font-size: 12px;

  &:hover,
  &:focus {
    background-color: ${p => p.theme.color.contextMenuHoverBackground};
    color: ${p => p.theme.color.contextMenuHoverForeground};
  }

  &:active {
    // TODO(petemill): Theme doesn't have a context menu interactive color,
    // make one and don't make entire element opaque.
    opacity: .8;
  }

  &:focus-visible {
    outline: solid 1px ${p => p.theme.color.brandBrave};
  }
`

export const Trigger = styled('div')`
  position: relative;
`

interface ButtonProps {
  isActive?: boolean
}

export const IconButton = styled('button')<ButtonProps>`
  appearance: none;
  position: relative;
  cursor: pointer;
  margin: 0;
  border: none;
  border-radius: 100%;
  background: none;
  padding: 2px;
  width: 20px;
  height: 20px;
  display: flex;
  align-items: center;
  justify-content: center;
  color: ${p => p.theme.color.text02};

  &:hover {
    color: ${p => p.theme.color.interactive02};
    background: rgba(160, 165, 235, 0.16);

  }
  &:focus-visible {
    box-shadow: 0 0 0 2px rgb(160, 165, 235);
  }
  ${p => p.isActive && css`
    color: rgba(76, 84, 210, 0.7);
  `}
  &:active {
    color: rgba(76, 84, 210, 0.7);
  }
`
