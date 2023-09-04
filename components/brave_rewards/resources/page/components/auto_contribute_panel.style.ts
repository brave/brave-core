/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

import * as mixins from '../../shared/lib/css_mixins'

export const root = styled.div``

export const description = styled.div`
  color: var(--brave-palette-neutral600);
  margin: 16px 0;
  line-height: 24px;
`

export const terms = styled.div`
  color: #3b3e4f;
  margin: 20px calc(-1 * var(--settings-panel-padding)) 0;
  padding: 0 15px 15px 32px;
  border-bottom: 1px solid rgba(184, 185, 196, 0.4);
  font-size: 13px;
`

export const publisherSupport = styled.div`
  display: flex;
  align-items: center;
  gap: 16px;

  background: #F8F9FA;
  border-radius: 8px;
  padding: 16px;
  color: #495057;
  font-weight: 400;
  font-size: 14px;
  line-height: 20px;
`

export const publisherCount = styled.div`
  flex: 0 1 auto;
  font-size: 40px;
  line-height: 40px;
`

export const remove = styled.span`
  button {
    ${mixins.buttonReset}
    cursor: pointer;
  }

  .icon {
    vertical-align: middle;
    height: 1em;
    width: auto;
    margin-top: -3px;
  }
`

export const showAll = styled.div`
  margin-top: 25px;
  text-align: right;

  button {
    ${mixins.buttonReset}
    cursor: pointer;
    font-weight: 600;
    font-size: 13px;
    line-height: 20px;
    letter-spacing: -0.01em;
    color: #4C54D2;
  }
`
