/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from '../../../../theme'

interface PageProps {
  isPrivate: boolean
}

export const Page = styled<PageProps, 'div'>('div')`
  box-sizing: border-box;
  -webkit-font-smoothing: antialiased;
  background: linear-gradient(${p => p.isPrivate ? '#381980' : '#5F0C8A'}, #0C041E);
  min-height: 100%;
  height: initial;
`

export const PageWrapper = styled<{}, 'main'>('main')`
  box-sizing: border-box;
  padding: 85px 15px;
  max-width: 950px;
  margin: 0 auto;
`
