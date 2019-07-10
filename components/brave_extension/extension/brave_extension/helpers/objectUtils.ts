/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

export const areObjectsEqual = (firstObj: {}, secondObj: {}): boolean => {
  const firstObjPropertyNames = Object.getOwnPropertyNames(firstObj)
  const secondObjPropertyNames = Object.getOwnPropertyNames(secondObj)

  if (firstObjPropertyNames.length !== secondObjPropertyNames.length) {
    return false
  }

  for (let i = 0; i < firstObjPropertyNames.length; i++) {
    const propertyName = firstObjPropertyNames[i]

    if (firstObj[propertyName] !== secondObj[propertyName]) {
      return false
    }
  }

  return true
}
