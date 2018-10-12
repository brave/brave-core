/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const StyledListContent = styled<{}, 'div'>('div')`
  padding: 0 25px;
`

export const StyledSupport = styled<{}, 'div'>('div')`
  background: ${p => p.theme.color.subtleBackground};
  margin-top: -9px;
  padding-top: 10px;
`

export const StyledSitesNum = styled<{}, 'div'>('div')`
  height: 50px;
  padding: 20px 25px;
  margin-top: -21px;
`

export const StyledDisabledContent = styled<{}, 'div'>('div')`
  padding: 0px 5px;
`

export const StyledHeading = styled<{}, 'span'>('span')`
  font-size: 22px;
  font-weight: normal;
  letter-spacing: 0;
  line-height: 28px;
`

export const StyledSitesLink = styled<{}, 'a'>('a')`
  float: right;
  color: #4C54D2;
  font-size: 13px;
  letter-spacing: 0;
`

export const StyledText = styled<{}, 'p'>('p')`
  color: #838391;
  font-size: 14px;
  font-family: ${p => p.theme.fontFamily.body};
  font-weight: 300;
  letter-spacing: 0;
  line-height: 28px;
`

export const StyledTotalContent = styled<{}, 'div'>('div')`
  position: relative;
  top: 8px;
  padding-right: 25px;

  @media (max-width: 366px) {
    top: 11px;
  }
`

export const StyledSupportSites = styled<{}, 'div'>('div')`
  position: relative;
  top: 9px;
  font-size: 16px;
  padding: 0 0 10px 25px;

  @media (max-width: 411px) {
    font-size: 14px;
  }
`
