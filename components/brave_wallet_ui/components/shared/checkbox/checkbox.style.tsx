// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled, { css, CSSProperties } from 'styled-components'

interface StyleProps {
  selected?: boolean
  size: 'big' | 'small'
  disabled?: boolean
}

const getLabelProps = (p: StyleProps) => {
  return css`
    --checkbox-border-size: 1px;
    --checkbox-box-size: ${p.size === 'big' ? 24 : 18}px;
    --checkbox-label-size: ${p.size === 'big' ? 16 : 14}px;
    --checkbox-box-spacing: ${p.size === 'big' ? 17 : 12}px;
    --checkbox-box-color: #696FDC;
    --checkbox-label-color: #B8B9C4;
    --checkbox-border-color: ${
      p.disabled
        ? '#EBECF0'
        : p.selected
          ? '#A1A8F2'
          : '#D1D1DB'
    };

    @media (prefers-color-scheme: dark) {
      --checkbox-box-color: #A1A8F2;
      --checkbox-label-color: #B8B9C4;
      --checkbox-border-color: ${
        p.disabled
          ? '#686978'
          : p.selected
            ? '#696FDC'
            : '#D1D1DB'
      };
    }

    &:focus, &:hover:not([disabled]) {
      outline: none;
      --checkbox-border-color: #A0A5EB;
      --checkbox-border-size: 3px;
    }
  `
}

export const StyledLabel = styled('label')<StyleProps & {
  alignItems?: CSSProperties['alignItems']
  justifyContent?: CSSProperties['justifyContent']
}>`
  ${getLabelProps};
  font-family: ${p => p.theme.fontFamily.body};
  display: flex;
  align-items: ${(p) => p?.alignItems ?? 'center'};
  justify-content: ${(p) => p?.justifyContent ?? 'center'};
  margin-bottom: 4px;
  color: var(--checkbox-label-color);
  font-size: var(--checkbox-label-size);
  cursor: pointer;
  &:focus {
    /* Focus style is on child 'box' */
    outline: none;
  }
`

export const StyledBox = styled('span')`
  border-radius: 4px;
  position: relative;
  text-align: center;
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 3px;
  flex-basis: var(--checkbox-box-size);
  width: var(--checkbox-box-size);
  height: var(--checkbox-box-size);
  color: var(--checkbox-box-color);
  margin: 8px;
  :after {
    /* Border provided by :after element so that transition of size is smooth without
      svg-resizing 'jumping' effect from resizing padding + border space at the same time */
    transition: border .1s ease-in-out;
    position: absolute;
    top: 0;
    left: 0;
    right: 0;
    bottom: 0;
    border: var(--checkbox-border-size) solid var(--checkbox-border-color);
    content: '';
    display: block;
    border-radius: 4px;
  }
`

export const StyledText = styled('span')<{ size?: 'big' | 'small' }>`
  flex: 1;
  padding-top: ${(p) => p?.size === 'big' ? '2px' : '1px'};
  letter-spacing: 0;
  display: flex;
`
