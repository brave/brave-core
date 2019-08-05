/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const StyledDateText = styled<{}, 'div'>('div')`
  font-family: ${p => p.theme.fontFamily.body};
  font-size: 14px;
  color: ${p => p.theme.color.text};
  display: inline-block;
`
export const StyledAdsDateRow = styled<{}, 'div'>('div')`
  cursor: pointer;
  white-space: nowrap;
`

export const StyledAdsDetailRow = styled<{}, 'div'>('div')`
  text-align: left;
  padding-top: 15px;
`

export const StyledHRDiv = styled<{}, 'div'>('div')`
  width: 94.5%;
  padding-left: 10px;
  display: inline-block;
`

export const StyledHR = styled<{}, 'hr'>('hr')`
  color: ${p => p.theme.color.subtle};
  background-color: ${p => p.theme.color.subtle};
  height: 2px;
  border: none;
`

export const StyledCaratIcon = styled<{}, 'div'>('div')`
  margin: 0;
  background: none;
  border: none;
  width: 16px;
  height: 16px;
  color: ${p => p.theme.color.subtle};
  padding: 1px;
  outline: none;
  display: inline-block;
  text-align: center;
`

export const StyledAdPortionTD = styled<{}, 'td'>('td')`
  font-family: ${p => p.theme.fontFamily.body};
  font-size: 14px;
  font-weight: 500;
  color: ${p => p.theme.color.text};
  border-bottom: none;
  padding: 5px 0;
  text-align: left;
`

export const StyledInnerStartTD = styled<{}, 'td'>('td')`
  font-family: ${p => p.theme.fontFamily.body};
  font-size: 14px;
  font-weight: 500;
  color: ${p => p.theme.color.text};
  border-bottom: none;
  padding: 12px 0;
  text-align: left;
  height: 0%;
`

export const StyledSpaceDiv = styled<{}, 'div'>('div')`
  min-width: 55px;
`
