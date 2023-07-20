/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

import * as mixins from '../../shared/lib/css_mixins'

export const panel = styled.div`
  --settings-panel-padding: 32px;

  margin-bottom: 32px;
  background: #FFFFFF;
  box-shadow:
    0px 0px 1px rgba(0, 0, 0, 0.11),
    0px 0.5px 1.5px rgba(0, 0, 0, 0.1);
  border-radius: 8px;
  padding: var(--settings-panel-padding);
  font-weight: 400;
  font-size: 14px;
  line-height: 20px;
  color: #212529;

  .layout-narrow & {
    --settings-panel-padding: 24px;
  }
`

export const header = styled.div`
  display: flex;
  align-items: center;
  gap: 16px;
`

export const title = styled.div`
  flex: 1 1 auto;
  font-weight: 600;
  font-size: 22px;
  line-height: 28px;
  color: var(--brave-palette-black);
`

export const config = styled.div`
  button {
    ${mixins.buttonReset}
    color: #A1A8F2;
    cursor: pointer;

    .icon {
      height: 20px;
      width: auto;
      vertical-align: middle;
      margin-top: 1px;
    }
  }

  &.active .icon {
    height: 12px;
    color: var(--brave-palette-neutral600);
  }
`

export const toggle = styled.div``

export const item = styled.div`
  padding: 16px 0;
  border-bottom: 1px solid #E9E9F4;
`

export const itemContent = styled.div`
  display: flex;
  align-items: center;
  justify-content: space-between;
  flex-wrap: wrap;
  gap: 8px;
`

export const itemLabel = styled.div`
  flex: 1 1 0%;
  font-weight: 500;
  font-size: 14px;
  line-height: 20px;
  color: #495057;
`

export const itemDetails = styled.div`
  padding-top: 8px;
  color: #686978;
`

export const amount = styled.div`
  color: #495057;
`

export const exchange = styled.span`
  margin-left: 8px;
  color: #868E96;

  .layout-narrow & {
    display: block;
    margin: 0;
  }
`

export const date = styled.div`
  background: #E9F0FF;
  border-radius: 6px;
  padding: 6px 10px;
  color: #4B4C5C;
`

export const table = styled.div`
  margin-top: 16px;
  color: #686978;

  .layout-narrow & {
    overflow-x: auto;
    font-size: 13px;
  }

  table {
    width: 100%;
    border-collapse: collapse;
  }

  th, td {
    padding: 10px 5px;
    text-align: left;
    border-bottom: solid 1px #e4e8ec;
  }

  th:first-child, td:first-child {
    padding-left: 0;
  }

  th:last-child, td:last-child {
    padding-right: 0;
  }

  th.number, td.number {
    text-align: right;
  }

  th {
    font-weight: 400;
    font-size: 12px;
    line-height: 18px;
    padding: 8px 5px;
    text-transform: uppercase;
    color: #931E93;
    border-bottom: 1px solid #931E93;
  }

  div.empty {
    margin: 64px 0 48px;
    font-weight: 400;
    font-size: 12px;
    line-height: 18px;
    text-align: center;
    color: #868E96;
  }
`

export const configHeader = styled.div`
  margin: 25px 0 8px;
  padding: 8px 0;
  font-weight: 400;
  font-size: 12px;
  line-height: 18px;
  color: #931E93;
  text-transform: uppercase;
  border-bottom: solid 1px #931E93;
`
