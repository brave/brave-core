/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'
import { LoaderIcon } from 'brave-ui/components/icons'

export const Container = styled.div`
  text-align: center;
  padding: 60px 0 50px 0;
  font-family: ${p => p.theme.fontFamily.body};
  font-size: 15px;
`

export const LoadIcon = styled(LoaderIcon)`
  color: ${p => p.theme.color.brandBat};
  height: 70px;
  width: 70px;
  margin-bottom: 34px;
  opacity: .8;
`
