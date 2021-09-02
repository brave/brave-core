/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const root = styled.div``

export const header = styled.div`
  background: rgba(255, 255, 255, 0.4);
  border-radius: 8px 8px 0 0;
  padding: 5px 17px;
  font-weight: 500;
  font-size: 14px;
  line-height: 22px;
  display: flex;

  > div:first-child {
    flex: 1 1 auto;
  }

  .brave-theme-dark & {
    background: rgb(23, 23, 31, 0.88);
  }
`

export const body = styled.div`
  color: var(--brave-palette-neutral900);
  background: var(--brave-palette-white);
  border-radius: 0 0 8px 8px;
  overflow: hidden;

  .brave-theme-dark & {
    background: rgb(23, 23, 31, 0.62);
    color: var(--brave-palette-grey000);
  }
`

export const dataTable = styled.div`
  padding: 0 16px 16px;

  table {
    width: 100%;
  }

  tr {
    border-bottom: solid 1px var(--brave-palette-neutral200);

    .brave-theme-dark & {
      border-color: var(--brave-palette-neutral600);
    }
  }

  td {
    font-size: 14px;
    line-height: 18px;
    padding: 10px 0;
  }

  td.amount {
    text-align: right;
    font-weight: 600;
    color: #6971d5;

    .currency {
      font-weight: normal;
      font-size: 12px;
      color: var(--brave-palette-neutral900);

      .brave-theme-dark & {
        color: var(--brave-palette-neutral400);
      }
    }
  }

  tr:first-child td {
    padding-top: 16px;
  }

  tr:nth-child(1) td.amount {
    color: #b13c7a;
  }

  tr:nth-child(2) td.amount {
    color: #b13c7a;
  }

  tr:nth-child(3) td.amount {
    color: #8d58c4;
  }

  td.exchange {
    text-align: right;
    font-size: 10px;
    color: var(--brave-palette-neutral600);

    .brave-theme-dark & {
      color: var(--brave-palette-grey400);
    }
  }
`
