/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*/

import styled from 'styled-components'

export const StyledWrapper = styled<{}, 'div'>('div')`
  font-family: ${p => p.theme.fontFamily.heading};
  padding-top: 10px;
  white-space: nowrap;
`

export const StyledLink = styled<{}, 'a'>('a')`
  cursor: pointer;
  display: inline-block;
  color: ${p => p.theme.palette.grey300};
  font-size: 16px;
  letter-spacing: 0;
`

export const StyledNotSelectedLink = styled<{}, 'span'>('span')`
  display: inline-block;
  color: ${p => p.theme.color.brandBat};
  font-size: 16px;
  letter-spacing: 0;
`

export const StyledText = styled<{}, 'span'>('span')`
  font-size: 16px;
  letter-spacing: 0;
  display: inline-block;
  color: ${p => p.theme.color.text};
  margin: 0 10px 0;
`

export const StyledAdsHistoryTitle = styled<{}, 'div'>('div')`
  font-size: 22px;
  font-weight: 600;
  color: ${p => p.theme.palette.magenta600};
  margin-bottom: 10px;
  line-height: 1.3;
`

export const StyledSeparatorText = styled<{}, 'span'>('span')`
  font-size: 16px;
  font-weight: 200;
  letter-spacing: 0;
  display: inline-block;
  color: ${p => p.theme.color.text};
`

export const StyledSubTitleText = styled<{}, 'div'>('div')`
  font-size: 30px;
  line-height: 1.2;
  margin-bottom: 10px;
  font-weight: 300;
`

export const StyledAdsInfoText = styled<{}, 'span'>('span')`
  font-size: 18px;
  color: ${p => p.theme.palette.magenta600};
  margin-right: 5px;
`

export const StyledAdsInfoTextWrapper = styled<{}, 'div'>('div')`
  line-height: 1.5;
`

export const StyledAdsPerHourText = styled<{}, 'span'>('span')`
  font-size: 18px;
  color: ${p => p.theme.palette.magenta600};
  font-weight: 600;
`

export const StyledAdsHeaderWrapper = styled<{}, 'div'>('div')`
  line-height: 1.5;
  width: 100%;
  display: flex;
  justify-content: space-between;
  align-items: baseline;
`

export const StyledAdsSaveFiltered = styled<{}, 'div'>('div')`
  text-align: right;
  line-height: 1.5;
  display: flex;
  justify-content: space-between;
`

export const StyledThumbDownFilter = styled<{}, 'div'>('div')`
  display: inline-block;
  width: 32px;
  height: 32px;
  margin-top: auto;
  padding: 4px;
  padding-top: 0px;
  cursor: pointer;
  color: ${p => p.theme.color.subtle};
`

export const StyledThumbDownNotSelectedFilter = styled<{}, 'div'>('div')`
  display: inline-block;
  width: 32px;
  height: 32px;
  color: ${p => p.theme.color.brandBat};
  padding: 4px;
  padding-top: 0px;
`
