/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled, { css } from 'styled-components'
import { Props } from './index'

const defaultBg = '#0c0d21'

export const StyledWrapper = styled.div`
  position: relative;
` as any

export const StyledArrow = styled.div`
  width: 0;
  height: 0;
  border-style: solid;
  position: absolute;
  z-index: 3;
  ${(p: Props) => p.position === 'bottom'
    ? css`
      top: -7px;
      left: calc(50% - 4px);
      border-width: 0 6.5px 8px 6.5px;
      border-color: transparent transparent ${(p: Props) => p.theme && p.theme.color ? p.theme.color.background : defaultBg} transparent;
    `
    : ''
  };

  ${(p: Props) => p.position === 'top'
    ? css`
      bottom: -7px;
      left: calc(50% - 4px);
      border-width: 8px 6.5px 0 6.5px;
      border-color: ${(p: Props) => p.theme && p.theme.color ? p.theme.color.background : defaultBg} transparent transparent transparent;
    `
    : ''
  };

  ${(p: Props) => p.position === 'left'
    ? css`
      top: 8px;
      right: -7px;
      border-width: 6.5px 0 6.5px 8px;
      border-color: transparent transparent transparent ${(p: Props) => p.theme && p.theme.color ? p.theme.color.background : defaultBg};
    `
    : ''
  };

  ${(p: Props) => p.position === 'right'
    ? css`
      top: 8px;
      left: -7px;
      border-width: 6.5px 8px 6.5px 0;
      border-color: transparent ${(p: Props) => p.theme && p.theme.color ? p.theme.color.background : defaultBg} transparent transparent;
    `
    : ''
  };
` as any

export const StyledArrowOutline = styled.div`
  width: 0;
  height: 0;
  border-style: solid;
  position: absolute;
  z-index: 2;
  ${(p: Props) => p.position === 'bottom'
    ? css`
      top: -9px;
      left: calc(50% - 4px);
      border-width: 0 6.5px 8px 6.5px;
      border-color: transparent transparent ${(p: Props) => p.theme && p.theme.color ? p.theme.color.border : defaultBg} transparent;
    `
    : ''
  };

  ${(p: Props) => p.position === 'top'
    ? css`
      bottom: -9px;
      left: calc(50% - 4px);
      border-width: 8px 6.5px 0 6.5px;
      border-color: ${(p: Props) => p.theme && p.theme.color ? p.theme.color.border : defaultBg} transparent transparent transparent;
    `
    : ''
  };

  ${(p: Props) => p.position === 'left'
    ? css`
      top: 8px;
      right: -9px;
      border-width: 6.5px 0 6.5px 8px;
      border-color: transparent transparent transparent ${(p: Props) => p.theme && p.theme.color ? p.theme.color.border : defaultBg};
    `
    : ''
  };

  ${(p: Props) => p.position === 'right'
    ? css`
      top: 8px;
      left: -9px;
      border-width: 6.5px 8px 6.5px 0;
      border-color: transparent ${(p: Props) => p.theme && p.theme.color ? p.theme.color.border : defaultBg} transparent transparent;
    `
    : ''
  };
` as any

export const StyledTooltip = styled.div`
  display: ${(p: any) => p.open ? 'block' : 'none'};
  position: absolute;
  border-radius: 4px;
  font-family: Muli;
  font-size: 14px;
  line-height: 1.71;
  letter-spacing: -0.1px;
  text-align: ${(p: Props) => p.theme && p.theme.align ? p.theme.align : 'center'};
  padding: ${(p: Props) => p.theme && p.theme.padding ? p.theme.padding : '4px'};
  width: ${(p: Props) => p.theme && p.theme.width ? p.theme.width : '150px'};
  box-shadow: ${(p: Props) => p.theme && p.theme.boxShadow ? p.theme.boxShadow : '1px 1px 5px 0 rgba(34, 35, 38, 0.43)'};
  background: ${(p: Props) => p.theme && p.theme.color ? p.theme.color.background : defaultBg};
  border: ${(p: Props) => p.theme && p.theme.border ? p.theme.border : `1px solid ${defaultBg}`};
  color: ${(p: Props) => p.theme && p.theme.color ? p.theme.color.text : '#fff'};

  ${(p: Props) => p.position === 'bottom'
    ? css`
      margin-top: ${(p: Props) => p.theme && p.theme.offSet ? p.theme.offSet : 14}px;
      top: 100%;
      left: -50%;
    `
    : ''
  };

  ${(p: Props) => p.position === 'top'
    ? css`
      margin-bottom: ${(p: Props) => p.theme && p.theme.offSet ? p.theme.offSet : 14}px;
      bottom: 100%;
      left: -50%;
    `
    : ''
  };

  ${(p: Props) => p.position === 'left'
    ? css`
      margin-right: ${(p: Props) => p.theme && p.theme.offSet ? p.theme.offSet : 14}px;
      right: 100%;
      top: -50%;
    `
    : ''
  };

  ${(p: Props) => p.position === 'right'
    ? css`
      margin-left: ${(p: Props) => p.theme && p.theme.offSet ? p.theme.offSet : 14}px;
      left: 100%;
      top: -50%;
    `
    : ''
  };
` as any
