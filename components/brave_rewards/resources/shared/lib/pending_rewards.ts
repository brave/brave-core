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
  console.log('--------------------------------------------------------------')

  console.log('nextPaymentDate.nextPaymentDate.asNumber: ', nextPaymentDate)

  if (typeof nextPaymentDate === 'number') {
    nextPaymentDate = new Date(nextPaymentDate)
  }

  console.log('nextPaymentDate.getFullYear: ', nextPaymentDate.getFullYear())
  console.log('nextPaymentDate.getMonth: ', nextPaymentDate.getMonth())
  console.log('nextPaymentDate.getDate: ', nextPaymentDate.getDate())

  // Round next payment date down to midnight local time
  nextPaymentDate = new Date(
    nextPaymentDate.getFullYear(),
    nextPaymentDate.getMonth(),
    nextPaymentDate.getDate())

  const now = Date.now()

  console.log('nextPaymentDate.AtMidnight: ', nextPaymentDate)
  console.log('nextPaymentDate.AtMidnight.getMonth(): ', nextPaymentDate.getMonth())
  console.log('Date(now).getMonth(): ', new Date(now).getMonth())

  // Only show pending days when payment date is within the current month
  if (nextPaymentDate.getMonth() !== new Date(now).getMonth()) {
    return ''
  }

  const delta = nextPaymentDate.getTime() - now
  console.log('delta: ', delta)

  console.log('--------------------------------------------------------------')

  const days = Math.ceil(delta / 24 / 60 / 60 / 1000)
  if (days < 1) {
    return ''
  }

  return pendingDaysFormatter.format(days)
}
