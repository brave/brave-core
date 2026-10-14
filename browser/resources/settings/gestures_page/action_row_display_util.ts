/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {GestureCommand} from './gesture_command.js'
import {hasIntParam} from './gesture_action_util.js'
import type {ActionRow} from './gesture_rows.js'

export function hasTextParam(command: number): boolean {
  return command === GestureCommand.NAVIGATE_TO_URL ||
    command === GestureCommand.SWITCH_TO_TAB
}

export function primaryActionRow(actionRows: ActionRow[]): ActionRow {
  return actionRows[0]
}

export function chainedActionRows(actionRows: ActionRow[]): ActionRow[] {
  return actionRows.slice(1)
}

export function hasChainedActions(actionRows: ActionRow[]): boolean {
  return chainedActionRows(actionRows).length > 0
}

export function primaryHasIntParam(actionRows: ActionRow[]): boolean {
  return hasIntParam(primaryActionRow(actionRows).command)
}

export function primaryHasTextParam(actionRows: ActionRow[]): boolean {
  return hasTextParam(primaryActionRow(actionRows).command)
}

// Which options `command`'s int-param dropdown should list.
export function intParamOptionsFor(
  command: number,
  openTargetOptions: Array<{ name: string, value: number }>,
  searchTargetOptions: Array<{ name: string, value: number }>,
  openSettingsPageOptions: Array<{ name: string, value: number }>):
  Array<{ name: string, value: number }> {
  if (command === GestureCommand.OPEN_LINK_IN ||
      command === GestureCommand.OPEN_IMAGE_IN) {
    return openTargetOptions
  }
  if (command === GestureCommand.SEARCH_SELECTED_TEXT) {
    return searchTargetOptions
  }
  return openSettingsPageOptions
}

export function primaryIntParamOptionsFor(
  actionRows: ActionRow[],
  openTargetOptions: Array<{ name: string, value: number }>,
  searchTargetOptions: Array<{ name: string, value: number }>,
  openSettingsPageOptions: Array<{ name: string, value: number }>):
  Array<{ name: string, value: number }> {
  return intParamOptionsFor(
    primaryActionRow(actionRows).command, openTargetOptions,
    searchTargetOptions, openSettingsPageOptions)
}

// Only `textParam` has an "unset" state; every other param type defaults
// to a value that is already valid on its own.
export function isActionIncomplete(
  command: number, textParam: string): boolean {
  return command === GestureCommand.UNASSIGNED ||
    (hasTextParam(command) && !textParam)
}

export function getCommandSelectClass(
  command: number, textParam: string): string {
  const classes = ['md-select']
  if (command === GestureCommand.UNASSIGNED) {
    classes.push('command-unassigned')
  }
  if (isActionIncomplete(command, textParam)) {
    classes.push('action-incomplete')
  }
  return classes.join(' ')
}

export function primaryCommandSelectClass(actionRows: ActionRow[]): string {
  const primary = primaryActionRow(actionRows)
  return getCommandSelectClass(primary.command, primary.textParam)
}

export function getUrlAttachClass(textParam: string): string {
  return textParam
    ? 'action-param param-attach'
    : 'action-param param-attach action-incomplete'
}

export function primaryUrlAttachClass(actionRows: ActionRow[]): string {
  return getUrlAttachClass(primaryActionRow(actionRows).textParam)
}

// Native `<option>` reflects the `<select>`'s value via its own `selected`
// attribute, not a one-way binding.
export function optionMatchesCommand(
  optionValue: number, command: number): boolean {
  return optionValue === command
}

export function optionMatchesPrimaryCommand(
  optionValue: number, actionRows: ActionRow[]): boolean {
  return optionMatchesCommand(
    optionValue, primaryActionRow(actionRows).command)
}

export function optionMatchesPrimaryOpenSettingsPage(
  optionValue: number, actionRows: ActionRow[]): boolean {
  return optionMatchesCommand(
    optionValue, primaryActionRow(actionRows).openSettingsPage)
}

// A pattern is shown as stored; a URL has its scheme stripped for display
// only, since it is always http/https.
export function getUrlButtonLabel(
  command: number, textParam: string, i18n: (key: string) => string): string {
  if (textParam) {
    return command === GestureCommand.SWITCH_TO_TAB
      ? textParam : textParam.replace(/^https?:\/\//, '')
  }
  return i18n(command === GestureCommand.SWITCH_TO_TAB
    ? 'braveGesturesSetPattern' : 'braveGesturesSetUrl')
}

export function primaryUrlButtonLabel(
  actionRows: ActionRow[], i18n: (key: string) => string): string {
  const primary = primaryActionRow(actionRows)
  return getUrlButtonLabel(primary.command, primary.textParam, i18n)
}
