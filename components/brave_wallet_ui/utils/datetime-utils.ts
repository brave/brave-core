// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { SerializableTimeDelta } from '../constants/types'

const monthMap = ['Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun', 'Jul', 'Aug', 'Sep', 'Oct', 'Nov', 'Dec']

export function formatDateAsRelative (date: Date, now: Date = new Date()) {
  // the difference in milliseconds
  const diff = now.getTime() - date.getTime()

  // convert diff to seconds
  const sec = Math.floor(diff / 1000)
  if (sec < 60) {
    return `${sec}s`
  }

  // convert diff to minutes
  const min = Math.floor(diff / (1000 * 60))
  if (min < 60) {
    return `${min}m`
  }

  // convert diff to hours
  const hour = Math.floor(diff / (1000 * 60 * 60))
  if (hour < 24) {
    return `${hour}h`
  }

  // convert diff to days
  const day = Math.floor(diff / (1000 * 60 * 60 * 24))
  if (day < 7) {
    return `${day}d`
  }

  // Do not display year for the current year
  if (date.getFullYear() === now.getFullYear()) {
    return `${monthMap[date.getMonth()]} ${date.getDate()}`
  }

  return `${monthMap[date.getMonth()]} ${date.getDate()}, ${date.getFullYear()}`
}

/**
 * Converts a SerializableTimeDelta to a JS time.
 * @param {SerializableTimeDelta} timeDelta
 * @return {!Date}
 */
 export function serializedTimeDeltaToJSDate (timeDelta: SerializableTimeDelta): Date {
  return new Date(Number(timeDelta.microseconds) / 1000)
}
