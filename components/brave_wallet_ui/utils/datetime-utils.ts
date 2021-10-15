import { MojoTime } from '../constants/types'

/**
 * Converts a mojo time to a JS time.
 * @param {!mojoBase.mojom.TimeDelta} mojoTime
 * @return {!Date}
 */
export function convertMojoTimeToJS (mojoTime: MojoTime) {
  return new Date(Number(mojoTime.microseconds) / 1000)
}

const monthMap = ['Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun', 'Jul', 'Aug', 'Sep', 'Oct', 'Nov', 'Dec']

export function formatDateAsRelative (date: Date) {
  const now = new Date()

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
