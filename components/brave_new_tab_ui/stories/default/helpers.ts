/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

export const getItems = (count: number) => {
  return Array
    .from({ length: count }, (value, key) => key)
    .map(key => ({ id: `item-${key}`, content: `${key}` }))
}

export const reorder = (list: any[], startIndex: number, endIndex: number) => {
  const result = Array.from(list)
  const [removed] = result.splice(startIndex, 1)
  result.splice(endIndex, 0, removed)

  return result
}

export const getListStyle = (isDraggingOver: boolean) => ({
  background: isDraggingOver ? 'lightblue' : 'lightgrey'
})
