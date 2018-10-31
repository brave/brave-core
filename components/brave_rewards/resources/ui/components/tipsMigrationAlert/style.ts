/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*/

import styled from 'styled-components'

export const StyledWrapper = styled<{}, 'div'>('div')`
  width: 100%;
  display: block;
  background: #E9F0FF;
  padding: 5px 30px;
  border-bottom-left-radius: 6px;
  border-bottom-right-radius: 6px;
`

export const StyledAlertIcon = styled<{}, 'div'>('div')`
  width: 57px;
  height: 57px;
  color: #15A4FA;
  display: inline-block;
`

export const StyledInfo = styled<{}, 'div'>('div')`
  font-size: 14px;
  letter-spacing: 0;
  line-height: 18px;
  padding: 12px 15px 0px 11px;
  vertical-align: top;
  display: inline-block;
  max-width: 387px;
`

export const StyledMessage = styled<{}, 'span'>('span')`
  color: #000;
  font-weight: 400;
  margin-right: 3px;
`

export const StyledMonthlyTips = styled<{}, 'span'>('span')`
  color: #696FDC;
  font-weight: 400;
  display: inline-block;
`

export const StyledReviewWrapper = styled<{}, 'div'>('div')`
  display: inline-block;
  vertical-align: top;
  margin: 20px 0 0 7px;
`

export const StyledReviewList = styled<{}, 'span'>('span')`
  color: #15A4FA;
  cursor: pointer;
  font-size: 14px;
  font-weight: 500;
  letter-spacing: 0;
  line-height: 18px;
`
