/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

import modalBackground from './assets/opt_in_modal_bg.svg'

export const root = styled.div`
  max-width: 335px;
  padding: 17px;
  background-color: var(--brave-palette-white);
  background-image: url(${modalBackground});
  background-repeat: no-repeat;
  background-position: 4px -11px;
  background-size: auto 200px;
  box-shadow: 0px 0px 16px rgba(99, 105, 110, 0.2);
  border-radius: 8px;

  &.tour-modal-wide {
    max-width: 710px;
    padding: 25px;
    background: linear-gradient(to right, #fff, #fff 50%, #f8f9fa 50%);
  }
`

export const content = styled.div`
  padding: 11px 5px 0;
  height: 410px;

  .tour-modal-wide > & {
    height: 300px;
    margin-top: 35px;
    margin-bottom: 22px;
  }
`
