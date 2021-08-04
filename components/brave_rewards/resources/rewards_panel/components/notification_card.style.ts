/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

import * as mixins from '../../shared/lib/css_mixins'

export const root = styled.div`
  background: var(--brave-palette-white);
  box-shadow: 0px 0px 16px rgba(99, 105, 110, 0.18);
  border-radius: 16px;
  padding: 11px 11px 19px;
  font-family: var(--brave-font-heading);

  strong {
    font-weight: 600;
  }

  .brave-theme-dark & {
    background: var(--brave-palette-grey800);
  }
`

export const header = styled.div`
  display: flex;
  justify-content: space-between;
`

export const date = styled.div`
  background: var(--brave-palette-blurple100);
  border-radius: 48px;
  padding: 3px 14px;
  font-weight: 600;
  font-size: 12px;
  line-height: 18px;
  color: var(--brave-palette-blurple600);

  .icon {
    height: 14px;
    width: auto;
    vertical-align: middle;
    margin-bottom: 2px;
    margin-right: 8px;
  }

  .brave-theme-dark & {
    background: #D9DBFF;
  }
`

export const close = styled.div`
  text-align: right;
  padding: 5px;

  button {
    ${mixins.buttonReset}
    color: var(--brave-palette-grey500);
    cursor: pointer;

    .brave-theme-dark & {
      color: var(--brave-palette-grey700);
    }

    &:active {
      color: var(--brave-palette-grey600);
    }
  }

  .icon {
    height: 12px;
    width: auto;
  }
`

const actionButtonMixin = `
  ${mixins.buttonReset}
  width: 100%;
  max-width: 244px;
  font-weight: 600;
  font-size: 13px;
  line-height: 20px;
  border-radius: 48px;
  padding: 10px;
  cursor: pointer;
`

export const content = styled.div`
  text-align: center;
  font-size: 14px;
  line-height: 20px;
`

export const title = styled.div`
  margin: 15px 0 7px;
  font-weight: 500;
  font-size: 18px;
  line-height: 20px;
  color: var(--brave-palette-neutral900);
  text-align: center;

  .brave-theme-dark & {
    color: var(--brave-palette-grey000);
  }

  .icon {
    height: 16px;
    width: auto;
    vertical-align: middle;
    margin-right: 5px;
    margin-bottom: 3px;
    color: var(--brave-palette-green500);
  }

  &.funding .icon {
    color: var(--brave-palette-green500);
  }

  &.information .icon {
    color: var(--brave-palette-blue500);
  }

  &.error .icon {
    color: var(--brave-palette-red500);
  }
`

export const body = styled.div`
  margin: 0 auto;
  max-width: 254px;

  .brave-theme-dark & {
    color: var(--brave-palette-grey400);
  }
`

export const action = styled.div`
  margin-top: 12px;
  text-align: center;

  button {
    ${actionButtonMixin}
    color: var(--brave-palette-white);
    background: var(--brave-color-brandBat);

    &:active {
      background: var(--brave-palette-blurple400);
    }

    .brave-theme-dark & {
      color: var(--brave-palette-grey000);
    }
  }
`

export const dismiss = styled.div`
  margin-top: 9px;
  text-align: center;

  button {
    ${actionButtonMixin}
    color: var(--brave-palette-neutral600);
    border: solid 1px var(--brave-palette-grey500);
    border-radius: 48px;

    &:active {
      background: var(--brave-palette-neutral000);
    }

    .brave-theme-dark & {
      color: var(--brave-palette-grey000);
      border-color: var(--brave-palette-grey700);

      &:active {
        background: rgba(255, 255, 255, .08);
      }
    }
`
