/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*/

import { Type } from './index'
import styled, { css } from 'styled-components'

interface StyleProps {
  buttonType?: Type
  hasIcon?: boolean
  disabled?: boolean
}

const getTypeStyle = (type?: Type, disabled?: boolean) => {
  let typeCss = null

  switch (type) {
    case 'tip':
      typeCss = css`
        height: 40px;
        color: #4C54D2;
        border-radius: 20px;
        font-size: 12px;
        border: 1px solid #A1A8F2;
      `
      break
    case 'opt-in':
      typeCss = css`
        height: 56px;
        border-radius: 28px;
        letter-spacing: .6px;
        border: 1px solid rgba(255, 255, 255, 0.35);

        &:hover {
          background: ${p => disabled ? 'inherit' : 'rgba(0, 0, 0, 0.05)'};
        }
      `
      break
    case 'cta-opt-in':
      typeCss = css`
        height: 56px;
        letter-spacing: .6px;
        background: ${p => p.theme.color.brandBrave};

        &:hover {
          background: ${p =>
            disabled
              ? p.theme.color.brandBrave
              : p.theme.brandBraveInteracting
          };
        }
      `
      break
  }

  return typeCss
}

export const StyledButtonWrapper = styled<StyleProps, 'button'>('button')`
  width: 100%;
  display: flex;
  color: white;
  font-size: 14px;
  border-radius: 28px;
  background: inherit;
  border: none;
  align-items: center;
  cursor: ${p => p.disabled ? 'default' : 'pointer'};
  ${p => getTypeStyle(p.buttonType, p.disabled)}

  &:focus {
    outline: 0;
  }
`

export const StyledButtonText = styled<StyleProps, 'span'>('span')`
  font-family: Poppins, sans-serif;
  font-weight: 600;
  text-transform: uppercase;
  letter-spacing: 0.39px;
  margin: ${p => p.hasIcon ? '-3px auto 0px' : '0 auto'};
`

export const StyledIcon = styled<{}, 'div'>('div')`
  display: inline-block;
  line-height: 0;
  height: 18px;
  width: 18px;
  vertical-align: bottom;
  margin: 0 -4px 0 6px;
`
