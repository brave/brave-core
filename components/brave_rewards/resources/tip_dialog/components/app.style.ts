/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

// TODO: Many of the colors used throughout are "palette" colors.
// Should we use them by name via css custom properties?

export const root = styled.div`
  display: flex;
  flex-flow: row wrap;

  font-family: Poppins, sans-serif;
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

  background: #fff;
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
    color: #D8D8D8;
  }

  .icon {
    display: block;
    height: 12px;
  }
`

export const error = styled.div`
  text-align: center;
  margin-top: 45%;
`

export const errorDetails = styled.div`
  margin-top: 10px;

  font-size: 12px;
`
