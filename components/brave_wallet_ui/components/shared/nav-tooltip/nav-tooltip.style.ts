// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

export const Tip = styled.div<{
  horizontalAlign?: 'left' | 'right' | 'center'
  isSwap?: boolean
  orientation: 'top' | 'bottom' | 'right'
  distance: number
}>`
  --y-tip-translation: ${(p) => p.orientation === 'top' ? `-${p.distance + 22}px` : p.orientation === 'bottom' ? `${p.distance + 22}px` : '0px'};
  --x-tip-translation: ${(p) => p.orientation === 'right' ? `${p.distance + 10}px` : '0px'};
  position: absolute;
  border-radius: 4px;
  left: ${(p) => p.horizontalAlign === 'left' ? '0px' : 'unset'};
  right: ${(p) => p.horizontalAlign === 'right' ? '0px' : 'unset'};
  transform: translateY(var(--y-tip-translation)) translateX(var(--x-tip-translation));
  padding: 8px 16px;
  color: ${(p) => p.theme.palette.white};
  background: ${(p) => p.isSwap ? 'var(--nav-tool-tip-background)' : p.theme.palette.black};
  z-index: 10;
  font-family: Poppins;
  font-size: 14px;
  line-height: 20px;
  letter-spacing: 0.01em;
  font-weight: 500;
`

export const Pointer = styled.div<{
  orientation: 'top' | 'bottom' | 'right'
  distance: number
  isSwap?: boolean
}>`
  --top-rotation: 180deg;
  --bottom-rotation: 0deg;
  --right-rotation: 270deg;
  --y-translation: ${(p) => p.orientation === 'top' ? `-${p.distance}px` : p.orientation === 'bottom' ? `${p.distance}px` : '0px'};
  --x-translation: ${(p) => p.orientation === 'right' ? `${p.distance}px` : '0px'};
  width: 0;
  height: 0;
  border-style: solid;
  position: absolute;
  transform: translateY(var(--y-translation)) translateX(var(--x-translation)) rotate(var(--${(p) => p.orientation}-rotation));
  border-width: 0 7px 8px 7px;
  z-index: 10;
  /* --nav-tool-tip-background is closer aligned with swap design theme */
  border-color: transparent transparent ${(p) => p.isSwap ? 'var(--nav-tool-tip-background)' : p.theme.palette.black} transparent;
`
