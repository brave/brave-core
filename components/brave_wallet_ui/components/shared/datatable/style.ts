/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { SortOrder } from '../../../constants/types'
import styled, { css } from 'styled-components'
import { Cell, Row } from './index'

import ArrowDownIcon from '../../../assets/svg-icons/arrow-down-fill.svg'
import ArrowUpIcon from '../../../assets/svg-icons/arrow-up-fill.svg'

export interface StyledThProps {
  customStyle?: {[key: string]: string}
  sortOrder: SortOrder
}

export const StyledTBody = styled.tbody`
  ::before {
    content: '';
    display: block;
    height: 24px;
  }
`

export const StyledNoContent = styled('div')<{}>`
  text-align: center;
  padding: 30px 0;
  color: ${p => p.theme.color.text03};
  font-size: 14px;
  font-family: Poppins;
`

export const StyledTable = styled('table')<{}>`
  min-width: 100%;
`

export const StyledTH = styled('th')<Partial<StyledThProps>>`
  text-align: left;
  font-family: Poppins;
  font-size: 12px;
  font-weight: 600;
  line-height: 18px;
  letter-spacing: 0.01em;
  border-bottom: ${(p) => `2px solid ${p.theme.color.disabled}`};
  color: ${(p) => p.sortOrder !== undefined ? p.theme.color.text02 : p.theme.color.text03};
  padding: 0 0 8px 0;

  ${p => p.customStyle
    ? css`
      ${p.customStyle}
    `
    : ''
  };
`

export const StyledTR = styled('tr')<Partial<Row>>`
  ${p => p.customStyle
    ? css`
      ${p.customStyle}
    `
    : ''
  };
`

export const StyledTD = styled('td')<Partial<Cell>>`
  vertical-align: middle;
  letter-spacing: 0.01em;
  font-family:Poppins;
  font-size: 14px;
  font-weight: 400;
  color: ${p => p.theme.color.text01};
  font-family: Poppins;
  font-size: 14px;
  line-height: 20px;
  padding-bottom: 16px;


  ${p => p.customStyle
    ? css`
      ${p.customStyle}
    `
    : ''
  };
`

export const ArrowDown = styled.div`
  display: inline-flex;
  width: 9px;
  height: 6px;
  background-repeat: no-repeat;
  background-image: url(${ArrowDownIcon});
  margin-right: 3px;
`

export const ArrowUp = styled.div`
  display: inline-flex;
  width: 9px;
  height: 6px;
  background-repeat: no-repeat;
  background-image: url(${ArrowUpIcon});
  margin-right: 3px;
`
