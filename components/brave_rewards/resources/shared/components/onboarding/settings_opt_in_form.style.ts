// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

import * as leo from '@brave/leo/tokens/css/variables'

export const root = styled.div`
  background-color: ${leo.color.white};
  border-radius: 16px;
  font-family: var(--brave-font-heading);
  text-align: center;
  padding: 44px;
`

export const icon = styled.div`
  .icon {
    height: 190px;
    width: 198px;
  }
`

export const heading = styled.div.attrs({
  'data-theme': 'light'
})`
  margin: 32px auto 0;
  max-width: 339px;
  font-weight: 500;
  font-size: 22px;
  line-height: 32px;
  color: ${leo.color.text.primary};
`

export const text = styled.div.attrs({
  'data-theme': 'light'
})`
  margin: 16px auto;
  max-width: 339px;
  color: ${leo.color.text.secondary};
  font-weight: 500;
  font-size: 14px;
  line-height: 24px;
`

export const enable = styled.div.attrs({
  'data-theme': 'light'
})`

  margin-top: 16px;
  color: ${leo.color.text.secondary};
  font-weight: 500;
  font-size: 14px;
  line-height: 24px;
  font-style: italic;

  .icon {
    height: 19px;
    width: auto;
    vertical-align: middle;
    margin-bottom: 3px;
  }

  button {
    margin-top: 16px;
    max-width: 339px;
    color: ${leo.color.white};
    background: ${leo.color.button.background};
    border-radius: 48px;
    padding: 12px 24px;
    font-weight: 600;
    font-size: 13px;
    line-height: 20px;
    letter-spacing: 0.03em;
  }
`

export const learnMore = styled.div.attrs({
  'data-theme': 'light'
})`
  margin: 20px 0 37px;
  text-align: center;

  a {
    color: ${leo.color.text.interactive};
    font-weight: 600;
    font-size: 13px;
    line-height: 20px;
    letter-spacing: 0.03em;
    text-decoration: none;
  }
`

export const terms = styled.div.attrs({
  'data-theme': 'light'
})`
  color: ${leo.color.text.tertiary};
  font-size: 11px;
  line-height: 16px;
  margin-bottom: -8px;

  a {
    color: ${leo.color.text.interactive};
    text-decoration: none;
  }
`
