/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

import * as leo from '@brave/leo/tokens/css/variables'
import * as mixins from '../../shared/lib/css_mixins'

export const root = styled.div`
  background: var(--brave-palette-white);
  box-shadow: 0px 4px 16px rgba(27, 29, 47, 0.08);
  border-radius: 16px;
  margin-top: 12px;
  padding: 18px 35px 26px;

  .brave-theme-dark & {
    background: #1E2029;
  }
`

export const heading = styled.div`
  display: flex;
  margin-bottom: 12px;
`

export const icon = styled.div`
  flex: 0 0 auto;
  margin-right: 9px;
  margin-top: 7px;

  img {
    height: 32px;
    width: auto;

    &.rounded {
      border-radius: 50%;
    }
  }
`

export const name = styled.div`
  flex: 1 1 auto;
  margin-top: 2px;
  font-weight: 500;
  font-size: 18px;
  line-height: 22px;
  color: var(--brave-palette-neutral900);

  .brave-theme-dark & {
    color: var(--brave-palette-grey000);
  }
`

export const status = styled.div`
  font-weight: 500;
  font-size: 12px;
  line-height: 18px;
  margin-top: 7px;
  color: var(--brave-palette-neutral700);
  display: flex;

  .brave-theme-dark & {
    color: var(--brave-palette-grey400);
  }
`

export const statusIndicator = styled.div`
  position: relative;
  border: 1px solid var(--brave-palette-neutral200);
  border-radius: 48px;
  padding: 3px 10px;
  display: flex;
  align-items: center;
  gap: 4px;

  &.verified {
    border-color: var(--brave-palette-grey500);
  }

  .brave-theme-dark & {
    border-color: var(--brave-palette-grey800);
  }
`

export const verifiedIcon = styled.div`
  --leo-icon-size: 16px;
  color: ${leo.color.neutral['30']};
`

export const refreshStatus = styled.div`
  padding-left: 10px;
  padding-top: 4px;

  .icon {
    height: 14px;
    width: 16px;
    margin-top: 2px;
    color: var(--brave-color-brandBat);
  }

  button {
    ${mixins.buttonReset}
    color: var(--brave-color-brandBatInteracting);
    font-weight: 600;
    cursor: pointer;
    text-align: left;

    &:hover {
      text-decoration: underline;
    }
  }

  .brave-theme-dark & {
    border-left-color: var(--brave-palette-grey800);

    button {
      color: var(--brave-palette-blurple300);
    }
  }
`

export const contribution = styled.div`
  border-top: solid 1px var(--brave-palette-neutral200);
  border-bottom: solid 1px var(--brave-palette-neutral200);
  font-size: 14px;
  line-height: 26px;
  color: var(--brave-palette-neutral600);
  padding: 6px 0 10px 0;

  .brave-theme-dark & {
    color: var(--brave-palette-grey600);
    border-color: var(--brave-palette-grey800);
  }

  &:empty {
    border: none;
    padding: 0;
  }
`

export const monthlyTip = styled.div`
  margin-top: 4px;
  display: flex;
  align-items: center;

  > * {
    flex: 1 1 auto;
  }

  > :last-child {
    text-align: right;
  }
`

export const monthlyTipAmount = styled.div`
  font-size: 14px;
  line-height: 22px;
  color: var(--brave-palette-neutral600);

  .amount {
    font-weight: 500;
    color: var(--brave-color-brandBatInteracting);

    .brave-theme-dark & {
      color: var(--brave-palette-blurple300);
    }
  }

  .currency {
    font-size: 12px;
  }

  .icon {
    width: 12px;
    height: auto;
    vertical-align: middle;
    margin-bottom: 1px;
    margin-right: 2px;
  }

  button {
    border: none;
    background: none;
    margin: 0;
    padding: 0;
    cursor: pointer;
    display: inline-flex;
    align-items: center;

    &:hover .icon {
      color: var(--brave-palette-neutral700);
    }

    .brave-theme-dark &:hover .icon {
      color: var(--brave-palette-grey400);
    }
  }
`

export const tipAction = styled.div`
  text-align: center;
  margin-top: 26px;

  button {
    width: 100%;
    background: var(--brave-color-brandBatInteracting);
    color: var(--brave-palette-white);
    padding: 10px;
    margin: 0;
    border: none;
    border-radius: 48px;
    font-weight: 600;
    font-size: 13px;
    line-height: 20px;
    cursor: pointer;

    &:hover {
      background: var(--brave-palette-blurple600);
    }

    &:disabled {
      background: #ACAFBB;
      opacity: 0.5;
      cursor: default;
    }
  }
`

export const unverifiedNote = styled.div`
  margin-top: 20px;
  background: #F0F7FC;
  border-radius: 8px;
  padding: 16px;
  color: #1D1F25;
  font-size: 12px;
  line-height: 18px;
  display: flex;
  align-items: center;
  gap: 16px;

  .icon {
    width: 17px;
    height: auto;
    color: #0F75C9;
    opacity: .65;
  }

  a {
    text-decoration: none;
  }

  .brave-theme-dark & {
    background: #042038;
    color: #eceff2;

    .icon {
      color: #2795ef;
    }

    a {
      color: var(--brave-palette-blurple400);
    }
  }
`
