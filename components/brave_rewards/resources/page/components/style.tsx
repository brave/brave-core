/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const TourPromoWrapper = styled('div')<{}>`
  margin-top: 30px;
`

export const PageWalletWrapper = styled.div`
  width: 373px;
`

export const PaymentStatus = styled.div`
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
