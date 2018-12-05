/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*/

import styled from 'styled-components'

export const StyledWrapper = styled<{}, 'div'>('div')`
  width: 100%;
  padding: 30px 25px 25px;
  font-family: Poppins, sans-serif;
  background-image: linear-gradient(140deg, #392DD1 0%, #8E2995 100%);
`

export const StyledTitle = styled<{}, 'div'>('div')`
  font-size: 16px;
  color: #F1F1F9;
  font-weight: 500;
  letter-spacing: 0;
`

export const StyledOff = styled<{}, 'div'>('div')`
  font-size: 16px;
  color: #FFFFFF;
  font-weight: 600;
  letter-spacing: 0;
  margin-left: 3px;
`

export const StyledText = styled<{}, 'div'>('div')`
  color: #F1F1F9;
  font-size: 14px;
  font-family: Muli,sans-serif;
  letter-spacing: 0;
  font-weight: 400;
  line-height: 20px;
  max-width: 276px;
  margin-top: 7px;
`

export const StyledLink = styled<{}, 'a'>('a')`
  cursor: pointer;
  margin-left: 4px;
  display: inline-block;
  color: #8D92E2;
`

export const StyledTitleWrapper = styled<{}, 'div'>('div')`
  display: flex;
`
