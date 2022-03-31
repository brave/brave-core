/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { SortOrder } from '../../../constants/types'
import styled, { css } from 'styled-components'
import { Cell, Row } from './index'

import ArrowDownIcon from '../../../assets/svg-icons/arrow-down-fill.svg'
import ArrowUpIcon from '../../../assets/svg-icons/arrow-up-fill.svg'

export interface StyleProps {
  customStyle?: {[key: string]: string}
  sortOrder: SortOrder
  sortable?: boolean
  stickyHeaders?: boolean
}

export const StyledWrapper = styled.div`
  width: 100%;
`

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
  position: relative;
  min-width: 100%;
  border-collapse: separate;
  border-spacing: 0;
`

export const StyledTHead = styled('thead')`
`

export const StyledTH = styled('th')<Partial<StyleProps>>`
  text-align: left;
  font-family: Poppins;
  font-size: 12px;
  font-weight: 600;
  line-height: 18px;
  letter-spacing: 0.01em;
  border-bottom: ${(p) => `2px solid ${p.theme.color.disabled}`};
  color: ${(p) => p.sortOrder !== undefined ? p.theme.color.text02 : p.theme.color.text03};
  padding: 10px 0 10px 0px;
  cursor: ${p => p.sortable ? 'pointer' : 'default'};
  position: ${p => p.stickyHeaders ? 'sticky' : 'relative'};
  background-color: ${(p) => p.theme.color.background02};
  top:  ${p => p.stickyHeaders ? 0 : 'inherit'};
  z-index: 2;
  
  &:hover {
    color: ${p => p.sortable ? p.theme.color.text02 : p.theme.color.text03};
  }

  ${p => p.customStyle
    ? css`
      ${p.customStyle}
    `
    : ''
  };

  &:last-child {
    padding-right: 20px;
  }
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
  padding: 0 0 16px 10px;

  ${p => p.customStyle
    ? css`
      ${p.customStyle}
    `
    : ''
  };

  &:last-child {
    padding-right: 20px;
  }
`
export const ArrowWrapper = styled.div`
  display: inline-flex;
  align-items: center;
  justify-content: center;
  width: 9px;
  height: 6px;
  margin-right: 3px;
`

export const ArrowBase = styled.div`
  display: inline-flex;
  width: 100%;
  height: 100%;
  background-repeat: no-repeat;
`
export const ArrowDown = styled(ArrowBase)`
  background-image: url(${ArrowDownIcon});
`

export const ArrowUp = styled(ArrowBase)`
  background-image: url(${ArrowUpIcon});
`
export const CellContentWrapper = styled.div<Partial<StyleProps>>`
  display: flex;
  align-items: center;
  ${p => p.customStyle
    ? css`
      ${p.customStyle}
    `
    : ''
  };
`
