/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const WalletContainer = styled<{}, 'div'>('div')`
  padding: 50px 20px;
  text-align: center;
`

export const WalletInner = styled<{}, 'div'>('div')`
  margin: 0 auto;
  max-width: 150px;
`

export const WalletTitle = styled<{}, 'h1'>('h1')`
  font-weight: 300;
`
