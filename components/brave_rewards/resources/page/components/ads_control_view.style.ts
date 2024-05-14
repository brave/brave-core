/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

import * as leo from '@brave/leo/tokens/css/variables'

import * as mixins from '../../shared/lib/css_mixins'

import selectArrow from '../assets/select_arrow.svg'

export const root = styled.div.attrs({
  'data-theme': 'light'
})`
  margin-top: 10px;
  color: ${leo.color.text.primary};

  --toggle-button-width: 40px;
  --toggle-button-handle-margin: 2px;

  div select {
    width: auto;
    -webkit-appearance: none;
    background:
      url(${selectArrow}) calc(100% - 11px) center no-repeat,
      ${leo.color.container.highlight};
    background-size: 11px;
    border-radius: 8px;
    border: none;
    color: inherit;
    font-size: 12px;
    line-height: 18px;
    padding: 5px 32px 5px 8px;

    &[disabled] {
      background:
        url(${selectArrow}) calc(100% - 11px) center no-repeat,
        ${leo.color.container.disabled};
      background-size: 11px;
      color: ${leo.color.text.disabled}
    }
  }
`

export const adTypeRow = styled.div`
  display: flex;
  gap: 22px;
  margin: 16px 0;
`

export const adTypeControls = styled.div`
  flex: 1 0;
  display: flex;
  flex-wrap: wrap;
  align-items: center;
  gap: 16px;
`

export const adTypeLabel = styled.div`
  flex: 0 0 124px;
  font-size: 12px;
  line-height: 24px;
  display: flex;
  align-items: center;
  gap: 9px;
`

export const adTypeInfo = styled.span.attrs({
  'data-theme': 'light'
})`
  --leo-icon-size: 14px;
  color: ${leo.color.icon.default};

  ${mixins.tooltipAnchor}

  .tooltip {
    top: 50%;
    transform: translateY(-50%);
    left: 100%;
    padding-left: 12px;
  }

  .layout-narrow & .tooltip {
    top: 100%;
    transform: none;
    left: -80px;
    padding-left: 0;
    padding-top: 12px;
  }
`

export const infoTooltip = styled.div.attrs({
  'data-theme': 'light'
})`
  ${mixins.tooltipContainer}

  width: 274px;
  padding: 16px;
  font-size: 14px;
  line-height: 24px;
  color: ${leo.color.text.primary};
  border-radius: 8px;
  box-shadow: 0px 4px 16px -2px rgba(0, 0, 0, 0.08);
  background: ${leo.color.white};

  &:before {
    height: 9px;
    width: 9px;
    top: calc(50% - 4px);
    left: -4px;
  }

  .layout-narrow &:before {
    top: -4px;
    left: 82px;
    padding-left: 0;
  }
`

export const adTypeToggle = styled.div``

export const adTypeConfig = styled.div``

export const adTypeCount = styled.div.attrs({
  'data-theme': 'light'
})`
  justify-self: end;
  font-size: 14px;
  line-height: 24px;

  .disabled {
    color: ${leo.color.text.tertiary};
  }
`
