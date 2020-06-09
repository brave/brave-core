/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

export let actions: any = null

export const getActions = () => actions

export const setActions = (newActions: any) => actions = newActions

export const formatDate = (date: number) => {
  return new Intl.DateTimeFormat(
    'default',
    {
      month: '2-digit',
      day: '2-digit',
      year: 'numeric',
      hour: '2-digit',
      minute: '2-digit',
      second: '2-digit'
    }).format(date)
}
