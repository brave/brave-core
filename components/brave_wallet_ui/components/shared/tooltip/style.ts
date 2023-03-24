// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled, { css, CSSProperties } from 'styled-components'

interface PositionProps {
  position: 'left' | 'right' | 'center'
  verticalPosition: 'above' | 'below'
}

export const TipAndChildrenWrapper = styled.div`
  position: relative;
  cursor: default;
  display: inline-block flex;
  align-items: center;
  justify-content: center;
  text-align: center;
`

export const TipWrapper = styled.div<PositionProps>`
  position: absolute;

  left: ${(p) => p.position === 'left' ? 0 : 'unset'};
  right: ${(p) => p.position === 'right' ? 0 : 'unset'};

  width: 100%;

  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  align-content: center;

  transform: 
    translateY(${
      (p) => p.verticalPosition === 'below'
        ? '8px'
        : '-105%'
    });

    z-index: 100;
`

export const Tip = styled.div<{
  isAddress?: boolean,
  maxWidth?: CSSProperties['maxWidth']
  minWidth?: CSSProperties['minWidth']
}>`
  border-radius: 4px;
  padding: 6px;

  font-family: Poppins;
  font-size: 12px;
  letter-spacing: 0.01em;
  
  width: ${(p) => p.isAddress ? '180px' : 'unset'};
  max-width: ${(p) => p?.maxWidth || '100%'};
  min-width: ${(p) => p?.minWidth || 'fit-content'};
  
  word-wrap: break-word;
  white-space: ${(p) => p.isAddress || p?.maxWidth ? 'pre-line' : 'nowrap'};
  word-break: ${(p) => p.isAddress ? 'break-all' : 'keep-all'};
  
  color: ${(p) => p.theme.palette.white};
  background-color: ${(p) => p.theme.palette.black};
  
  @media (prefers-color-scheme: dark) {
    border: 1px ${(p) => p.theme.color.divider01} solid;
  }
`

export const Pointer = styled.div<PositionProps>`
  width: 0;
  height: 0;
  border-style: solid;
  border-width: 0 7px 8px 7px;

  border-color: transparent transparent ${(p) => p.theme.palette.black} transparent;

  @media (prefers-color-scheme: dark) {
    border-color: transparent transparent ${(p) => p.theme.color.divider01} transparent;
  }
  
  ${(p) => p.position === 'center' && css`
    margin: 0 auto;
  `}

  transform: 
    translateX(${(p) => p.position === 'center'
      ? '0'
      : p.position === 'right'
        ? '-25px'
        : '25px'
    })
    translateY(${(p) => p.verticalPosition === 'above'
      ? '-1px'
      : '1px'
    })
    rotate(${(p) => p.verticalPosition === 'below'
      ? '0deg'
      : '180deg'
    });

`

export const ActionNotification = styled(Tip)`
  background: ${p => p.theme.palette.blurple500};
`
