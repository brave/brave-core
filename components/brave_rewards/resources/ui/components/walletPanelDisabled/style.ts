/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*/

import styled from 'styled-components'

export const StyledWrapper = styled<{}, 'div'>('div')`
  text-align: center;
  font-family: Poppins, sans-serif;
  margin-top: 7px;
  padding-bottom: 20px;
  background: linear-gradient(-180deg, rgba(255,255,255,1) 0%, rgba(233, 235, 255,1) 99%);
` as any

export const StyledBatLogo = styled<{}, 'span'>('span')`
  display: block;
  margin: -10px auto 0px;
  width: 150px;
  height: 115px;
  padding: 20px 0 15px;
` as any

export const StyledTitle = styled<{}, 'span'>('span')`
  color: #4B4C5C;
  font-size: 28px;
  font-weight: normal;
  letter-spacing: 0;
  line-height: 22px;
  display: block;
` as any

export const StyledSubTitle = styled<{}, 'span'>('span')`
  color: #4C54D2;
  font-size: 18px;
  font-weight: 600;
  text-align: center;
  letter-spacing: -0.23px;
  line-height: 22px;
  display: block;
  margin: 10px 0px 5px;
` as any

export const StyledText = styled<{}, 'span'>('span')`
  display: block;
  color: #4B4C5C;
  font-size: 16px;
  font-weight: normal;
  letter-spacing: 0;
  line-height: 24px;
  width: 285px;
  margin: 0 auto;
  font-family: Muli, sans-serif;
` as any

export const StyledArrowIcon = styled<{}, 'span'>('span')`
  top: 2px;
  position: relative;
  font-weight: 600;
` as any

export const StyledButtonWrapper = styled<{}, 'div'>('div')`
  width: 269px;
  margin: 18px auto 15px;
` as any

export const StyledServiceText = styled<{}, 'span'>('span')`
  color: #4A4A4A;
  font-size: 14px;
  font-weight: normal;
  text-align: center;
  letter-spacing: 0;
  line-height: 18px;
  width: 290px;
  margin: 0 auto;
  display: block;
  font-family: Muli, sans-serif;
` as any

export const StyledServiceLink = styled<{}, 'a'>('a')`
  cursor: pointer;
  color: #73CBFF;
  font-weight: 500;
` as any
