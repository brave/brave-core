/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const root = styled.div`
  max-width: 335px;
  min-height: 530px;
  padding: 17px;
  background-color: var(--brave-palette-white);
  box-shadow: 0px 0px 16px rgba(99, 105, 110, 0.2);
  border-radius: 8px;
  display: flex;
  flex-direction: column;

  &.tour-modal-wide {
    max-width: 710px;
    min-height: 450px;
    padding: 25px;
    background: linear-gradient(to right, #fff, #fff 50%, #f8f9fa 50%);
  }
`

export const content = styled.div`
  padding: 11px 5px 0;
  flex: 1 1 auto;
  display: flex;

  .tour-modal-wide > & {
    margin-top: 20px;
    margin-bottom: 10px;
  }
`
