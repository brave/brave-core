/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const root = styled.div`
  display: flex;
  align-items: flex-start;
  gap: 18px;
  background: var(--color-systemfeedback-info-background);
  border-radius: 8px;
  padding: 24px;

  .icon {
    color: var(--color-systemfeedback-info-icon);
  }

  &.warn {
    background: var(--color-systemfeedback-warning-background);

    .icon {
      color: var(--color-systemfeedback-warning-icon);
    }
  }

  &.error {
    background: var(--color-systemfeedback-error-background);

    .icon {
      color: var(--color-systemfeedback-error-icon);
    }
  }
`

export const icon = styled.div`
  flex: 0 0 26px;
  opacity: 0.25;
`

export const content = styled.div`
  flex: 1 1 auto;
  display: flex;
  flex-direction: column;
  gap: 4px;
`

export const title = styled.div`
  font-weight: 600;
  font-size: 12px;
  line-height: 18px;
  color: var(--color-text-primary);
`

export const text = styled.div`
  font-size: 12px;
  line-height: 18px;
  color: var(--color-text-primary);

  > div:not(:first-child) {
    margin-top: 1em;
  }
`
