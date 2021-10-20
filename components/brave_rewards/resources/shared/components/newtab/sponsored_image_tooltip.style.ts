/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

import { tooltipMixin } from './tooltip_mixin'
import { buttonReset } from '../../lib/css_mixins'

export const root = styled.div`
  ${tooltipMixin}
  font-family: var(--brave-font-heading);

  button {
    font-family: var(--brave-font-heading);
  }

  a {
    color: inherit;
    text-decoration: underline;
  }

  animation-name: fade-in;
  animation-delay: .5s;
  animation-duration: .5s;
  animation-fill-mode: both;

  @keyframes fade-in {
    from { opacity: 0; }
    to { opacity: 1; }
  }
`

export const close = styled.div`
  text-align: right;
  padding: 9px 10px 0;
  line-height: 12px;

  button {
    ${buttonReset}
    cursor: pointer;
    padding: 2px;
  }

  .icon {
    height: 12px;
    width: auto;
    vertical-align: middle;
  }
`

export const body = styled.div`
  padding: 0 24px 24px;
`

export const title = styled.div`
  margin-bottom: 8px;
  font-weight: 600;
  font-size: 15px;
  line-height: 20px;
  letter-spacing: 0.04em;

  .icon {
    height: 22px;
    width: auto;
    margin-right: 7px;
    margin-bottom: 2px;
    vertical-align: middle;
  }
`

export const action = styled.div`
  margin-top: 8px;
  margin-bottom: 8px;

  button {
    ${buttonReset}
    width: 100%;
    padding: 10px;
    font-weight: 600;
    font-size: 13px;
    line-height: 20px;
    cursor: pointer;
    background: rgba(255, 255, 255, 0.24);
    backdrop-filter: blur(16px);
    border-radius: 48px;

    .icon {
      height: 1.5em;
      width: auto;
      vertical-align: middle;
    }

    &:hover {
      background: rgba(255, 255, 255, 0.36);
    }

    &:active {
      background: rgba(255, 255, 255, 0.48);
    }
  }
`
