/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*/

import styled from 'styled-components'

interface StyleProps {
  src?: string
  success?: boolean
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
  font-family: "Poppins", sans-serif;
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
  padding-top: 220px;
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
  margin-top: 245px;
  padding-left: 5px;
  display: flex;
  flex-direction: column;
  align-items: center;
`

export const StyledCloseIcon = styled<{}, 'span'>('span')`
  width: 55px;
  color: #E2052A;
  display: block;
  cursor: pointer;
  margin-bottom: 40px;
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
