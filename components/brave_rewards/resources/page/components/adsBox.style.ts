/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const paymentStatus = styled.div`
  margin-bottom: 8px;
  color: var(--brave-palette-neutral900);
  font-size: 14px;
  line-height: 24px;

  a {
    color: var(--brave-palette-blurple500);
    font-weight: 600;
    text-decoration: none;
  }

  > div {
    background: #E8F4FF;
    border-radius: 4px;
    padding: 6px 21px;
    display: flex;
    gap: 6px;
    font-weight: 600;
  }

  .rewards-payment-amount {
    .plus {
      margin-right: 2px;
    }
  }

  .rewards-payment-pending {
    background: #E8F4FF;

    .icon {
      color: var(--brave-palette-blue500);
      height: 16px;
      width: auto;
      vertical-align: middle;
      margin-bottom: 1px;
    }
  }

  .rewards-payment-completed {
    background: #E7FDEA;
    align-items: center;

    .icon {
      height: 20px;
      width: auto;
      vertical-align: middle;
    }
  }

  .rewards-payment-check-status {
    display: block;
  }
`

export const needsBrowserUpdateView = styled.div`
  display: flex;
  align-items: center;
  justify-content: start;
  background: #FDF1F2;
  padding: 5px;
  border-radius: 6px;
`

export const needsBrowserUpdateIcon = styled.div`
  width: 30px;
  height: 30px;
  margin: 8px;
  color: var(--brave-palette-red500);
`

export const needsBrowserUpdateContent = styled.div`
  display: block;
`

export const needsBrowserUpdateContentHeader = styled.div`
  margin: 5px;
  font-size: 16px;
  font-weight: 600;
  color: var(--brave-palette-neutral800);
`

export const needsBrowserUpdateContentBody = styled.div`
  margin: 5px;
  font-size: 15px;
  color: var(--brave-palette-neutral800);
`
