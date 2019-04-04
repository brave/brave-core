/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*/

import styled from '../../../theme'
import * as CSS from 'csstype'

interface StyleProps {
  bg?: string
  src?: string
  success?: boolean
  monthly?: string
  logoBgColor?: CSS.Color
}

export const StyledOuterWrapper = styled<{}, 'div'>('div')`
  display: flex;
`

export const StyledWrapper = styled<{}, 'div'>('div')`
  display: flex;
  position: fixed;
  top: 0;
  left: 0;
  width: 100%;
  height: 100%;
  background: rgba(12,13,33,0.85);
  font-family: ${p => p.theme.fontFamily.body};
  justify-content: center;
`

export const StyledHeaderText = styled<{}, 'span'>('span')`
  color: #D1D1DB;
  font-size: 38px;
  font-weight: 600;
  letter-spacing: 0;
  line-height: 28px;
`

export const StyledOverlayTop = styled<{}, 'div'>('div')`
  display: flex;
  flex-direction: row;
  padding-top: 110px;
`

export const StyledOverlayContent = styled<{}, 'div'>('div')`
  display: block;
`

export const StyledIconWrapper = styled<StyleProps, 'span'>('span')`
  flex: 1 0 0;
  margin-top: ${p => p.success ? 0 : '-25px'}
`

export const StyledIcon = styled<{}, 'span'>('span')`
  width: 90px;
  margin-top: -7px;
  margin-right: 9px;
  display: inline-block;
`

export const StyledMessage = styled<StyleProps, 'div'>('div')`
  flex: 9 0 0;
  padding-top: 10px;
  text-align: ${p => p.monthly ? 'center' : 'inherit'};
  margin-right: ${p => p.success ? 0 : '-10px'}
`

export const StyledProviderImage = styled<StyleProps, 'div'>('div')`
  width: 90px;
  height: 90px;
  padding: 0 20px;
  border-radius: 50%;
  margin-right: 25px;
  background-repeat: no-repeat;
  background-size: 90px;
  background-image:url(${p => p.src ? p.src : ''});
`

export const StyledImageBorder = styled<{}, 'div'>('div')`
  position: relative;
  top: 0;
  left: -20px;
  width: 90px;
  height: 90px;
  border-radius: 50%;
  border: 5px solid #ffffff;
`

export const StyledFailWrapper = styled<{}, 'div'>('div')`
  margin-top: 110px;
  padding-left: 5px;
  display: flex;
  flex-direction: column;
  align-items: center;
`

export const StyledCloseIcon = styled<{}, 'span'>('span')`
  color: #FFF;
`

export const StyledClose = styled<{}, 'button'>('button')`
  top: 20px;
  right: 20px;
  position: absolute;
  background: none;
  border: none;
  cursor: pointer;
  width: 24px;
  height: 24px;
  color: #FFF;
  padding: 0;
  z-index: 2;
`

export const StyledFailTitle = styled<{}, 'span'>('span')`
  color: #FFFFFF;
  font-size: 28px;
  font-weight: 600;
  letter-spacing: 0;
  line-height: 28px;
  display: block;
  margin-bottom: 10px;
`

export const StyledFailMsg = styled<{}, 'span'>('span')`
  color: #FFFFFF;
  font-size: 16px;
  font-family: "Muli", sans-serif;
  font-weight: normal;
  letter-spacing: 0;
  line-height: 28px;
  display: block;
  text-align: center;
  width: 249px;
`

export const StyledBackgroundCurve = styled<{}, 'div'>('div')`
  position: fixed;
  top: 0;
  left: -19px;
  width: 105%;
  height: 480px;
  background: #191A2E;
  border-bottom-left-radius: 50%;
  border-bottom-right-radius: 140%;
`

export const StyleSubHeaderText = styled<{}, 'div'>('div')`
  font-size: 16px;
  font-family: "Muli", sans-serif;
  font-weight: normal;
  letter-spacing: 0;
  line-height: 28px;
  display: block;
  margin: 10px 0 0 5px;
`
export const StyledLetter = styled<StyleProps, 'div'>('div')`
  border: 6px solid #fff;
  border-radius: 50%;
  width: 102px;
  height: 102px;
  background: ${p => p.logoBgColor || '#DE4D26'};
  overflow: hidden;
  margin: -12px 25px 0 0;
  color: #fff;
  text-align: center;
  line-height: 90px;
  font-size: 65px;
  text-transform: uppercase;
`

export const StyledLogoImage = styled<StyleProps, 'div'>('div')`
  width: 90px;
  height: 90px;
  background: url(${p => p.bg}) no-repeat;
  background-size: cover;
`

export const StyledLogoWrapper = styled<{}, 'div'>('div')`
  padding-right: 25px;
  flex-basis: 217px;
`

export const StyledLogoBorder = styled<StyleProps, 'div'>('div')`
  border: 6px solid #fff;
  border-radius: 50%;
  width: 102px;
  height: 102px;
  margin-top: -12px;
  background: ${p => p.bg || '#DE4D26'};
  overflow: hidden;
`

export const StyledMonthlyInfo = styled<{}, 'div'>('div')`
  color: #fff;
`

export const StyledDomainText = styled<{}, 'span'>('span')`
  display: block;
  font-size: 22px;
  margin: 10px 0 25px;
  font-weight: normal;
  letter-spacing: 0;
  line-height: 32px;
`

export const StyledDateText = styled<{}, 'span'>('span')`
  display: block;
  font-size: 16px;
  font-weight: normal;
  text-align: center;
  letter-spacing: 0;
  line-height: 28px;
`

export const StyledDate = styled<{}, 'span'>('span')`
  display: block;
  font-size: 16px;
  font-weight: 500;
  text-align: center;
  letter-spacing: 0;
  line-height: 28px;
  color: ${p => p.theme.color.brandBrave};
`
