/* This Source Code Form is subject to the terms of the Mozilla Public
* License. v. 2.0. If a copy of the MPL was not distributed with this file.
* You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const StyledAdStatThumbUpIcon = styled<{}, 'div'>('div')`
  display: inline-block;
  width: 32px;
  height: 32px;
  padding: 4px;
  cursor: pointer;
`

export const StyledAdStatThumbUpFilledIcon = styled(StyledAdStatThumbUpIcon)`
  color: red;
`

export const StyledAdStatThumbDownIcon = styled<{}, 'div'>('div')`
  display: inline-block;
  width: 32px;
  height: 32px;
  margin-top: auto;
  padding: 4px;
  cursor: pointer;
`

export const StyledAdStatThumbDownFilledIcon = styled(StyledAdStatThumbDownIcon)`
  color: red;
`
