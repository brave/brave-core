/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

import * as leo from '@brave/leo/tokens/css/variables'

import * as mixins from '../../shared/lib/css_mixins'

export const root = styled.div`
  display: flex;
  gap: 18px;
  justify-content: space-between;
  align-items: flex-start;
`

export const selector = styled.div`
  --amount-selector-border-color: ${leo.color.divider.subtle};

  display: flex;
  gap: 1px;
  border-radius: 8px;
  overflow: hidden;
  border: solid 1px ${leo.color.divider.subtle};
  background: ${leo.color.divider.subtle};

  button {
    ${mixins.buttonReset}
    background: ${leo.color.container.background};
    padding: 12px 15px;
    min-width: 50px;
    font-weight: 600;
    font-size: 13px;
    line-height: 20px;
    color: ${leo.color.text.secondary};

    &:hover {
      cursor: pointer;
    }
  }

  button.selected {
    background: ${leo.color.container.background};
    color: ${leo.color.text.interactive};
  }
`

export const amount = styled.div``

export const primary = styled.div`
  display: flex;
  align-items: center;
  justify-content: flex-end;
  gap: 8px;

  .insufficient-balance & div {
    color: ${leo.color.systemfeedback.errorIcon};
  }
`

export const primarySymbol = styled.div`
  font-size: 14px;
  line-height: 24px;
  color: ${leo.color.text.secondary};
`

export const primaryAmount = styled.div`
  font-weight: 500;
  font-size: 28px;
  line-height: 44px;
  color: ${leo.color.text.primary};
`

export const primaryLabel = styled.div`
  font-weight: 600;
  font-size: 14px;
  line-height: 24px;
  color: ${leo.color.text.secondary};
`

export const customInput = styled.div`
  input {
    border: none;
    margin: 0;
    background: ${leo.color.container.highlight};
    border-radius: 8px;
    padding: 6px 0;
    width: 3em;
    text-align: center;
    font-weight: 500;
    font-size: 28px;
    line-height: 32px;
    color: ${leo.color.text.primary};
  }
`

export const secondary = styled.div`
  display: flex;
  align-items: center;
  justify-content: flex-end;
  gap: 10px;
`

export const secondaryAmount = styled.div`
  font-size: 14px;
  line-height: 24px;
  color: ${leo.color.text.secondary};
`

export const swap = styled.div`
  display: flex;
  align-items: center;
  margin-bottom: 3px;

  button {
    ${mixins.buttonReset}
    height: 16px;
    width: 16px;
    cursor: pointer;
  }
`
