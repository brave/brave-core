/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

 export let actions: any = null

 export const getActions = () => actions
 export const setActions = (newActions: any) => actions = newActions

/**
 * Gets the user's operating system name
 */
export const getDefaultDeviceName = () => {
  const userAgent = window.navigator.userAgent
  const currentPlatform = window.navigator.platform
  const macosVariants = ['Macintosh', 'MacIntel', 'MacPPC', 'Mac68K']
  const windowsVariants = ['Win32', 'Win64', 'Windows', 'WinCE']
  const iOSVariants = ['iPhone', 'iPad', 'iPod']
  const androidVariants = ['Android']

  let system

  if (macosVariants.includes(currentPlatform)) {
    system = 'Mac'
  } else if (windowsVariants.includes(currentPlatform)) {
    system = 'Windows'
  } else if (iOSVariants.includes(currentPlatform)) {
    system = 'iOS'
  } else if (androidVariants.includes(userAgent)) {
    system = 'Android'
  } else {
    system = 'Linux'
  }
  return `${system} Desktop`
}
