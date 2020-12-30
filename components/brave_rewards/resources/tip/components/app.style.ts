/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const root = styled.div`
  display: flex;
  flex-flow: row wrap;
  font-family: var(--brave-font-heading);
  font-size: 14px;
  line-height: 22px;

  a {
    text-transform: none;
    text-decoration: none;
  }

  strong {
    font-weight: 600;
  }
`

export const banner = styled.div`
  flex: 99999999 1 0;
  min-width: 300px;
  min-height: 454px;
`

export const form = styled.div`
  position: relative;
  flex: 1 0 364px;
  min-height: 454px;
  background: var(--brave-palette-white);
`

export const close = styled.div`
  button {
    position: absolute;
    top: 11px;
    right: 10px;
    padding: 10px;
    background: none;
    border: none;
    border-radius: 6px;
    cursor: pointer;
  }

  .icon {
    display: block;
    height: 12px;
    color: var(--brave-palette-grey500);
  }
`
