/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'
import { EmoteSadIcon } from 'brave-ui/components/icons'

export const Container = styled.div`
  text-align: center;
  min-height: 329px;
  padding-top: 30%;
`

export const SadIcon = styled(EmoteSadIcon)`
  color: ${p => p.theme.color.brandBat};
  height: 40px;
  width: 40px;
`

export const Text = styled.div`
  margin-top: 20px;
  color: ${p => p.theme.palette.grey800};
`

export const Details = styled.div`
  margin-top: 10px;
  color: ${p => p.theme.palette.grey600};
  font-size: .8em;
`
