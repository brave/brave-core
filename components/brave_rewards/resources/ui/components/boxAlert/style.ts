/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*/

import { Type } from './index'
import styled from 'styled-components'

interface StyleProps {
  modal?: boolean
  type?: Type
}

export const StyledWrapper = styled<{}, 'div'>('div')`
  width: 100%;
  display: flex;
  background: #E9F0FF;
  padding: 5px 30px;
  border-bottom-left-radius: 6px;
  border-bottom-right-radius: 6px;
  box-shadow: 0 1px 12px 0 rgba(99,105,110,0.18);
`

export const StyledAlertIcon = styled<{}, 'div'>('div')`
  width: 57px;
  height: 57px;
  color: #15A4FA;
`

export const StyledInfo = styled<StyleProps, 'div'>('div')`
  font-size: 14px;
  letter-spacing: 0;
  line-height: 18px;
  vertical-align: top;
  max-width: 387px;
  display: flex;
  align-items: center;
  justify-content: center;
  padding-left: 5px;
`

export const StyledMessage = styled<StyleProps, 'span'>('span')`
  color: #000;
  margin-right: ${p => p.modal ? 5 : 3}px;
  font-weight: ${p => p.modal ? 500 : 400};
  font-size: ${p => p.modal ? '22px' : 'inherit'};
`

export const StyledMonthlyTips = styled<StyleProps, 'span'>('span')`
  color: #696FDC;
  font-weight: ${p => p.modal ? 500 : 400};
  display: inline-block;
  font-size: ${p => p.modal ? '22px' : 'inherit'};
`

export const StyledReviewWrapper = styled<{}, 'div'>('div')`
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

export const StyledModalContent = styled<{}, 'div'>('div')`
  display: block;
`

export const StyledTipsIcon = styled<{}, 'div'>('div')`
  width: 20%;
  vertical-align: top;
  margin-top: -33px;
  display: inline-block;
`

export const StyledModalInfo = styled<{}, 'div'>('div')`
  width: 80%;
  padding-left: 20px;
  display: inline-block;
`

export const StyledListMessage = styled<{}, 'div'>('div')`
  display: block;
  font-size: 16px;
  font-weight: 600;
  margin-top: 30px;
`

export const StyledList = styled<{}, 'ul'>('ul')`
  display: block;
  font-size: 14px;
  font-weight: 300;
  padding-left: 20px;
`

export const StyledListItem = styled<{}, 'li'>('li')`
  display: block;
  display: list-item;
  line-height: 28px;
  list-style-type: disc;
`

export const StyledButton = styled<{}, 'div'>('div')`
  width: 235px;
  margin: 40px auto 0 auto;
`

export const StyledButtonContainer = styled<{}, 'div'>('div')`
  width: 100%;
`
