/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

import formBackgroundURL from '../assets/form_background.svg'
import formBackgroundDarkURL from '../assets/form_background_dark.svg'

import successBackground1URL from '../assets/success_background_1.svg'
import successBackground1DarkURL from '../assets/success_background_1_dark.svg'

import successBackground2URL from '../assets/success_background_2.svg'
import successBackground2DarkURL from '../assets/success_background_2_dark.svg'

import successBackground3URL from '../assets/success_background_3.svg'
import successBackground3DarkURL from '../assets/success_background_3_dark.svg'

import * as mixins from '../lib/button_mixins'

export const root = styled.div`
  display: flex;
  flex-direction: column;
  gap: 24px;

  --form-background-url: url(./${formBackgroundURL});
  --form-background-gradient-color-1: #FFFFFF;
  --form-background-gradient-color-2: rgba(255, 255, 255, 0);
  --success-background-1-url: url(./${successBackground1URL});
  --success-background-2-url: url(./${successBackground2URL});
  --success-background-3-url: url(./${successBackground3URL});

  @media (prefers-color-scheme: dark) {
    --form-background-url: url(./${formBackgroundDarkURL});
    --form-background-gradient-color-1: #1E2025;
    --form-background-gradient-color-2: rgba(30, 32, 37, 0);
    --success-background-1-url: url(./${successBackground1DarkURL});
    --success-background-2-url: url(./${successBackground2DarkURL});
    --success-background-3-url: url(./${successBackground3DarkURL});
  }
`

export const card = styled.div`
  background:
    linear-gradient(
      0deg,
      var(--form-background-gradient-color-1) 0,
      var(--form-background-gradient-color-1) 88px,
      var(--form-background-gradient-color-2)  192px
    ),
    no-repeat center bottom 88px / cover var(--form-background-url);

  overflow: hidden;
  box-shadow: 0px 4px 16px -1px rgba(0, 0, 0, 0.07);
  border-radius: 16px;
  display: flex;
  flex-direction: column;
`

export const title = styled.div`
  padding: 32px;
  font-weight: 500;
  font-size: 22px;
  line-height: 32px;
  color: var(--leo-color-text-primary);
`

export const inputPanel = styled.div`
  margin: 0 16px;
  background: var(--leo-color-container-background);
  border-radius: 8px;
  display: flex;
  flex-direction: column;
  padding: 8px;
`

export const controls = styled.div`
  margin: 24px;
  display: flex;
  flex-direction: column;
  gap: 32px;

  &:empty {
    margin: 0;
  }
`

export const buttons = styled.div`
  margin: 18px 32px 32px;
  display: flex;
  flex-direction: column;
  gap: 8px;

  button:first-child {
    ${mixins.primaryButton}
  }

  button:not(:first-child) {
    ${mixins.secondaryButton}
  }

  &:empty {
    margin-top: 0;
  }
`

export const terms = styled.div`
  text-align: center;
  font-weight: 400;
  font-size: 11px;
  line-height: 16px;
  color: var(--leo-color-text-secondary);

  a {
    color: var(--leo-color-text-primary);
    text-decoration: none;
  }
`

export const successCard = styled.div`
  background:
    no-repeat center top 40px / 435px auto var(--success-background-1-url),
    no-repeat center top / 100% auto var(--success-background-2-url),
    no-repeat center top / cover var(--success-background-3-url);
  overflow: hidden;
  box-shadow: 0px 4px 16px -1px rgba(0, 0, 0, 0.07);
  border-radius: 16px;
  padding-top: 238px;
  display: flex;
  flex-direction: column;
  gap: 9px;
  text-align: center;
`

export const successTitle = styled.div`
  margin: 0 67px;
  font-weight: 600;
  font-size: 14px;
  line-height: 24px;
  color: var(--leo-color-systemfeedback-success-icon);
`

export const successText = styled.div`
  margin: 0 72px 48px;
  font-weight: 600;
  font-size: 24px;
  line-height: 36px;
  color: var(--leo-color-text-interactive);
`
