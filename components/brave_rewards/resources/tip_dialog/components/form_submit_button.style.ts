/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const root = styled.div`
  button {
    width: 100%;
    padding: 19px 0;

    cursor: pointer;
    border: none;
    background: #4C54D2;
    font-size: 14px;
    line-height: 21px;
    font-weight: 600;
    color: #fff;
  }

  button:active {
    background: #686fdc;
  }

  .icon {
    height: 16px;
    vertical-align: middle;
    margin-right: 5px;
    margin-top: var(--icon-margin-top, 0);
  }
`
