/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const content = styled.div``

export const spinner = styled.div`
  position: absolute;
  top: 0;
  left: 0;
  bottom: 0;
  right: 0;
  display: flex;
  justify-content: center;
  align-items: center;
  visibility: hidden;

  .icon {
    height: 1.4em;
    width: auto;
  }
`

export const root = styled.div`
  button {
    position: relative;
  }

  button[disabled] {
    ${content} {
      visibility: hidden;
    }

    ${spinner} {
      visibility: visible;
    }
  }
`
