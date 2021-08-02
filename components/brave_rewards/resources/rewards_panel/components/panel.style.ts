/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const root = styled.div`
  width: 400px;
  background: #fafcff;
  padding: 15px 13px;
  font-family: var(--brave-font-heading);

  .brave-theme-dark & {
    background: #17171f;
  }
`
