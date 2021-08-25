import { MojoTime } from '../constants/types'

/**
 * Converts a mojo time to a JS time.
 * @param {!mojoBase.mojom.TimeDelta} mojoTime
 * @return {!Date}
 */
export function convertMojoTimeToJS (mojoTime: MojoTime) {
  return new Date(Number(mojoTime.microseconds) / 1000)
}
