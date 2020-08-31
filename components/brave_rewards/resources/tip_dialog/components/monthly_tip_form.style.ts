/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const root = styled.div`
  display: flex;
  flex-direction: column;
  height: 100%;
`

export const main = styled.div`
  flex: 1 1 auto;
  align-self: center;
  padding: 0 32px;
  width: 100%;
  max-width: 450px;
`

export const amounts = styled.div`
  padding-top: 24px;
  margin-top: 11px;

  border-top: 1px solid rgba(174, 177, 194, 0.5);
}
`

export const footer = styled.div``

export const submit = styled.div`
  --icon-margin-top: -4px;
`
