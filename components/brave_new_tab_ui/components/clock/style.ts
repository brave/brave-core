/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from '../../../theme'

const StyledClock = styled<{}, 'div'>('div')`
  box-sizing: border-box;
  line-height: 1;
  user-select: none;
  display: flex;
  cursor: default;
  -webkit-font-smoothing: antialiased;
  font-family: Poppins, -apple-system, system-ui, "Segoe UI", Roboto, Oxygen, Ubuntu, Cantarell, "Open Sans", "Helvetica Neue", sans-serif;
  color: rgb(255, 255, 255);
`

const StyledTime = styled<{}, 'span'>('span')`
  box-sizing: border-box;
  letter-spacing: 0;
  font-size: 75px;
  font-weight: 300;
  color: inherit;
  font-family: inherit;
  display: inline-flex;
`

const StyledPeriod = styled<{}, 'span'>('span')`
  box-sizing: border-box;
  color: inherit;
  font-family: inherit;
  display: inline-block;
  font-size: 20px;
  letter-spacing: -0.2px;
  font-weight: 300;
  margin-top: 8px;
  margin-left: 3px;
  vertical-align: top;
`

const StyledTimeSeparator = styled<{}, 'span'>('span')`
  box-sizing: border-box;
  color: inherit;
  font-size: inherit;
  font-family: inherit;
  font-weight: 300;
  /* center colon vertically in the text-content line */
  margin-top: -0.1em;
`

export {
  StyledClock,
  StyledTime,
  StyledPeriod,
  StyledTimeSeparator
}
