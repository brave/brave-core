/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

const pendingDaysFormatter = new Intl.NumberFormat(undefined, {
  style: 'unit',
  // @ts-ignore: "unit" property not yet supported by TS
  unit: 'day',
  // @ts-ignore: "unitDisplay" property not yet support by TS
  unitDisplay: 'long',
  maximumFractionDigits: 0
})

export function getDaysUntilRewardsPayment (nextPaymentDate: number | Date) {
  if (typeof nextPaymentDate === 'number') {
    nextPaymentDate = new Date(nextPaymentDate)
  }

  // Only show pending days when payment date is within the current month
  if (nextPaymentDate.getMonth() !== new Date().getMonth()) {
    return ''
  }

  const delta = nextPaymentDate.getTime() - Date.now()
  const days = Math.ceil(delta / 24 / 60 / 60 / 1000)
  if (days < 1) {
    return ''
  }

  return pendingDaysFormatter.format(days)
}
