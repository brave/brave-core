/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import 'chrome://resources/brave/leo.bundle.js'

import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'
import {I18nMixin} from 'chrome://resources/cr_elements/i18n_mixin.js'

import {getTemplate} from './gesture_action_chain_group.html.js'
import type {CommandOption} from './commands_cache.js'
import type {ActionRow} from './gesture_rows.js'
import {
  defaultActionRow, dropAction, applyCommandToActionRows, hasIntParam
} from './gesture_action_util.js'
import {
  primaryActionRow, chainedActionRows, hasChainedActions,
  primaryHasIntParam, primaryHasTextParam, intParamOptionsFor,
  primaryIntParamOptionsFor, hasTextParam, getCommandSelectClass,
  primaryCommandSelectClass, getUrlAttachClass, primaryUrlAttachClass,
  optionMatchesCommand, optionMatchesPrimaryCommand,
  optionMatchesPrimaryOpenSettingsPage, getUrlButtonLabel, primaryUrlButtonLabel
} from './action_row_display_util.js'
import {GestureCommand} from './gesture_command.js'

export interface EditUrlClickEventDetail {
  actionRows: ActionRow[]
  targetAction: ActionRow
}

// Every caller passes the same option lists at every call site; bundled so
// each binds one property instead of repeating all 4.
export interface OptionSets {
  commandOptions: CommandOption[]
  openTargetOptions: Array<{ name: string, value: number }>
  searchTargetOptions: Array<{ name: string, value: number }>
  openSettingsPageOptions: Array<{ name: string, value: number }>
}

const GestureActionChainGroupElementBase = I18nMixin(PolymerElement)

// Renders one gesture's action chain. Mutates `actionRows` itself, except
// `edit-url-click`, since that dialog is shared state the host must own.
export class GestureActionChainGroupElement extends
  GestureActionChainGroupElementBase {
  static get is() {
    return 'gesture-action-chain-group'
  }

  static get template() {
    return getTemplate()
  }

  static get properties() {
    return {
      actionRows: { type: Array, notify: true, value: () => [] },
      optionSets: {
        type: Object, value: () => ({
          commandOptions: [], openTargetOptions: [], searchTargetOptions: [],
          openSettingsPageOptions: [],
        })
      },
      hasConflict: { type: Boolean, value: false },
    }
  }

  declare actionRows: ActionRow[]
  declare optionSets: OptionSets
  declare hasConflict: boolean

  // Set on drag start, read on drop; cleared either way so a drop outside
  // any valid target, or in a different chain group entirely, is a no-op.
  private draggedAction_: ActionRow | null = null

  private primaryActionRow_(actionRows: ActionRow[]): ActionRow {
    return primaryActionRow(actionRows)
  }

  private chainedActionRows_(actionRows: ActionRow[]): ActionRow[] {
    return chainedActionRows(actionRows)
  }

  private hasChainedActions_(actionRows: ActionRow[]): boolean {
    return hasChainedActions(actionRows)
  }

  private hasTextParam_(command: number): boolean {
    return hasTextParam(command)
  }

  private hasIntParam_(command: number): boolean {
    return hasIntParam(command)
  }

  private primaryHasIntParam_(actionRows: ActionRow[]): boolean {
    return primaryHasIntParam(actionRows)
  }

  private primaryHasTextParam_(actionRows: ActionRow[]): boolean {
    return primaryHasTextParam(actionRows)
  }

  private intParamOptionsFor_(command: number):
    Array<{ name: string, value: number }> {
    return intParamOptionsFor(
      command, this.optionSets.openTargetOptions,
      this.optionSets.searchTargetOptions,
      this.optionSets.openSettingsPageOptions)
  }

  private primaryIntParamOptionsFor_(actionRows: ActionRow[]):
    Array<{ name: string, value: number }> {
    return primaryIntParamOptionsFor(
      actionRows, this.optionSets.openTargetOptions,
      this.optionSets.searchTargetOptions,
      this.optionSets.openSettingsPageOptions)
  }

  private getCommandSelectClass_(command: number, textParam: string): string {
    return getCommandSelectClass(command, textParam)
  }

  private primaryCommandSelectClass_(
    actionRows: ActionRow[], hasConflict: boolean): string {
    return primaryCommandSelectClass(actionRows, hasConflict)
  }

  private getUrlAttachClass_(textParam: string): string {
    return getUrlAttachClass(textParam)
  }

  private primaryUrlAttachClass_(actionRows: ActionRow[]): string {
    return primaryUrlAttachClass(actionRows)
  }

  private optionMatchesCommand_(optionValue: number, command: number):
    boolean {
    return optionMatchesCommand(optionValue, command)
  }

  private optionMatchesPrimaryCommand_(
    optionValue: number, actionRows: ActionRow[]): boolean {
    return optionMatchesPrimaryCommand(optionValue, actionRows)
  }

  private optionMatchesPrimaryOpenSettingsPage_(
    optionValue: number, actionRows: ActionRow[]): boolean {
    return optionMatchesPrimaryOpenSettingsPage(optionValue, actionRows)
  }

  private getUrlButtonLabel_(command: number, textParam: string): string {
    return getUrlButtonLabel(command, textParam, this.i18n.bind(this))
  }

  private primaryUrlButtonLabel_(actionRows: ActionRow[]): string {
    return primaryUrlButtonLabel(actionRows, this.i18n.bind(this))
  }

  private onPrimaryCommandChanged_(e: Event) {
    this.actionRows = applyCommandToActionRows(
      Number((e.target as HTMLSelectElement).value), this.actionRows)
  }

  private onPrimaryOpenSettingsPageChanged_(e: Event) {
    const openSettingsPage = Number((e.target as HTMLSelectElement).value)
    this.actionRows = this.actionRows.map(
      (action, index) => index === 0 ? { ...action, openSettingsPage } :
        action)
  }

  private onEditPrimaryUrlClick_() {
    this.dispatchEvent(new CustomEvent<EditUrlClickEventDetail>(
      'edit-url-click', {
        bubbles: true, composed: true,
        detail: {
          actionRows: this.actionRows,
          targetAction: this.actionRows[0],
        },
      }))
  }

  // Only shown when there's at least one chained action, so there's always
  // a next action to promote into the primary slot this one vacates.
  private onRemovePrimaryActionClick_() {
    this.actionRows = this.chainedActionRows_(this.actionRows)
  }

  private onPrimaryActionDragStart_(e: DragEvent) {
    this.draggedAction_ = this.actionRows[0]
    e.dataTransfer!.effectAllowed = 'move'
    // Some drag implementations only treat the drag as valid, and fire
    // `drop`, once `dataTransfer` actually carries data.
    e.dataTransfer!.setData('text/plain', '')
  }

  private onPrimaryActionDrop_(e: DragEvent) {
    e.preventDefault()
    this.onDrop_(this.actionRows[0])
  }

  private onChainedActionCommandChanged_(
    e: Event & { model: { chainedAction: ActionRow } }) {
    const command = Number((e.target as HTMLSelectElement).value)
    this.actionRows = this.actionRows.map((action) =>
      action === e.model.chainedAction ? defaultActionRow(command) : action)
  }

  private onChainedActionOpenSettingsPageChanged_(
    e: Event & { model: { chainedAction: ActionRow } }) {
    const openSettingsPage = Number((e.target as HTMLSelectElement).value)
    this.actionRows = this.actionRows.map((action) =>
      action === e.model.chainedAction ? { ...action, openSettingsPage } :
        action)
  }

  private onEditChainedUrlClick_(
    e: Event & { model: { chainedAction: ActionRow } }) {
    this.dispatchEvent(new CustomEvent<EditUrlClickEventDetail>(
      'edit-url-click', {
        bubbles: true, composed: true,
        detail: {
          actionRows: this.actionRows,
          targetAction: e.model.chainedAction,
        },
      }))
  }

  private onRemoveChainedActionClick_(
    e: Event & { model: { chainedAction: ActionRow } }) {
    this.actionRows =
      this.actionRows.filter((action) => action !== e.model.chainedAction)
  }

  private onChainedActionDragStart_(
    e: DragEvent & { model: { chainedAction: ActionRow } }) {
    this.draggedAction_ = e.model.chainedAction
    e.dataTransfer!.effectAllowed = 'move'
    e.dataTransfer!.setData('text/plain', '')
  }

  private onChainedActionDrop_(
    e: DragEvent & { model: { chainedAction: ActionRow } }) {
    e.preventDefault()
    this.onDrop_(e.model.chainedAction)
  }

  private onDrop_(target: ActionRow) {
    this.actionRows =
      dropAction(this.actionRows, this.draggedAction_, target)
    this.draggedAction_ = null
  }

  // Allows a drop; without this, the browser rejects it. Do not add
  // mid-drag reordering here; it has twice crashed the browser process.
  private onActionDragOver_(e: DragEvent) {
    e.preventDefault()
    if (e.dataTransfer) {
      e.dataTransfer.dropEffect = 'move'
    }
  }

  // A no-op if a drop already cleared `draggedAction_`; otherwise clears a
  // drag that ended outside any valid target.
  private onActionDragEnd_() {
    this.draggedAction_ = null
  }

  private onAddChainedActionClick_(e: Event) {
    // Rebuilding the row below replaces this button's DOM, so blur it
    // first or focus would land wherever the replacement happens to be.
    ;(e.target as HTMLElement).blur()
    this.actionRows = [
      ...this.actionRows,
      defaultActionRow(GestureCommand.UNASSIGNED),
    ]
  }
}

declare global {
  interface HTMLElementTagNameMap {
    'gesture-action-chain-group': GestureActionChainGroupElement
  }
}

customElements.define(
  GestureActionChainGroupElement.is, GestureActionChainGroupElement)
