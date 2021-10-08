/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

interface PageProps {
  isPrivate: boolean
}

export const Page = styled('div')<PageProps>`
  box-sizing: border-box;
  -webkit-font-smoothing: antialiased;
  background: linear-gradient(${p => p.isPrivate ? '180deg, #0C041E -8.41%, #4E21B7 98.85%' : '#5F0C8A, #0C041E'});
  min-height: 100vh;
  height: initial;
`

export const PageWrapper = styled('main')<{}>`
  box-sizing: border-box;
  padding: 85px 15px;
  max-width: 950px;
  margin: 0 auto;
`
