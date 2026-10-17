/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import '../settings_shared.css.js'
import 'chrome://resources/brave/leo.bundle.js'

import {PrefsMixin} from '/shared/settings/prefs/prefs_mixin.js'
import {I18nMixin} from 'chrome://resources/cr_elements/i18n_mixin.js'
import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'
import type {DomRepeatEvent} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'
import {SettingsViewMixin} from '../settings_page/settings_view_mixin.js'

import {getTemplate} from './gestures_page.html.js'
import {GestureCommand, SPECIAL_COMMAND_I18N_KEYS} from './gesture_command.js'
import {CommandsCache} from './commands_cache.js'
import type { Command } from '../commands.mojom-webui.js'
import type {CommandOption} from './commands_cache.js'
import {svgPathData} from './custom_gesture_svg_path.js'
import {
  OpenSettingsPage, OPEN_SETTINGS_PAGE_ORDER, OPEN_SETTINGS_PAGE_PATHS
} from './open_settings_page.js'
import {
  toActionDicts, defaultActionRow, actionRowsFromDicts, moveAction,
  applyCommandToActionRows, IDC_OPTIONS
} from './gesture_action_util.js'
import type {ActionDict} from './gesture_action_util.js'
import { withUrlScheme } from './url_scheme_util.js'
import type {
  ActionRow, DisallowedSiteRow, MouseRockerActionRow, MouseWheelActionRow
} from './gesture_rows.js'
import {
  rowCommandSequence, findConflictingGestures, conflictDescription
} from './gesture_command_conflict.js'
import {
  anyCustomGestureMatchesDirectionalGestureKey,
  customGestureShapeConflictDescription
} from './gesture_shape_conflict.js'
import {
  GESTURES_DIRECTIONAL_GESTURE_MAPPINGS_PREF,
  buildDirectionalGestureMappingActionRows
} from './directional_gesture_mapping_util.js'
import type {DirectionalGestureMappingActionRow} from './directional_gesture_mapping_util.js'
import {
  GESTURES_CUSTOM_GESTURES_PREF, DRAWN_CUSTOM_GESTURE_ROW,
  buildCustomGestureActionRows
} from './custom_gesture_util.js'
import type {CustomGestureActionRow, CustomGestureEntry} from './custom_gesture_util.js'
import type {Point} from './custom_gesture_geometry.js'

// Shown as the page's literal relative path (see `OPEN_SETTINGS_PAGE_PATHS`),
// not a translated label.
const OPEN_SETTINGS_PAGE_OPTIONS: Array<{ name: string, value: OpenSettingsPage }> =
  OPEN_SETTINGS_PAGE_ORDER.map(
    (value: OpenSettingsPage) => ({ name: OPEN_SETTINGS_PAGE_PATHS[value], value }))

// One instance per page load is fine, since the backend just adds
// another listener remote.
const commandsCache = new CommandsCache()

const GESTURES_DISALLOWED_SITES_PREF = 'brave.gestures.disallowed_sites'
const GESTURES_SENSITIVITY_IN_PIXELS_PREF =
  'brave.gestures.sensitivity_in_pixels'
const GESTURES_MOUSE_ROCKER_LEFT_PREF = 'brave.gestures.mouse_rocker_left_action'
const GESTURES_MOUSE_ROCKER_RIGHT_PREF = 'brave.gestures.mouse_rocker_right_action'
const GESTURES_MOUSE_WHEEL_MODIFIER_BUTTON_PREF =
  'brave.gestures.mouse_wheel_modifier_button'
const GESTURES_MOUSE_WHEEL_UP_PREF = 'brave.gestures.mouse_wheel_up_action'
const GESTURES_MOUSE_WHEEL_DOWN_PREF = 'brave.gestures.mouse_wheel_down_action'
const GESTURES_MOUSE_WHEEL_SENSITIVITY_IN_PIXELS_PREF =
  'brave.gestures.mouse_wheel_sensitivity_in_pixels'

// Keep in sync with features::kMinStrokePixels's default in
// brave/components/gestures/core/common/features.cc.
const DEFAULT_SENSITIVITY_IN_PIXELS = 30
const SENSITIVITY_SNAP_THRESHOLD_IN_PIXELS = 2

// Keep in sync with features::kMaxDirectionalStrokes's default in
// brave/components/gestures/core/common/features.cc.
const MAX_DIRECTIONAL_STROKES = 4

// Keep in sync with features::kMinWheelDelta's default in
// brave/components/gestures/core/common/features.cc.
const DEFAULT_MOUSE_WHEEL_SENSITIVITY_IN_PIXELS = 0
const MOUSE_WHEEL_SENSITIVITY_SNAP_THRESHOLD_IN_PIXELS = 2

// Keep in sync with `kMouseWheelModifierButton`'s registered default in
// `prefs_registration.cc`.
const DEFAULT_MOUSE_WHEEL_MODIFIER_BUTTON = 2 /* MouseWheelModifierButtonType::kRight */

const SettingsGesturesPageBase =
  SettingsViewMixin(PrefsMixin(I18nMixin(PolymerElement)))

class SettingsGesturesPage extends SettingsGesturesPageBase {
  static get is() {
    return 'settings-gestures-page'
  }

  static get template() {
    return getTemplate()
  }

  // Complex observers, rather than reading prefs in `ready`, since `prefs`
  // is populated asynchronously by chrome.settingsPrivate, so reading it
  // synchronously at `ready` time, before the fetch resolves, would hit
  // `PrefsMixin.getPref`'s "Pref is missing" assert. These fire once each
  // dependency is first defined, and again on every subsequent change.
  static get observers() {
    return [
      'onDirectionalGestureMappingsPrefChanged_(' +
      'prefs.brave.gestures.directional_gestures.value)',
      'onDisallowedSitesPrefChanged_(' +
      'prefs.brave.gestures.disallowed_sites.value)',
      'onCustomGesturesPrefChanged_(' +
      'prefs.brave.gestures.custom_gestures.value)',
      'onMouseRockerLeftActionPrefChanged_(' +
      'prefs.brave.gestures.mouse_rocker_left_action.value)',
      'onMouseRockerRightActionPrefChanged_(' +
      'prefs.brave.gestures.mouse_rocker_right_action.value)',
      'onMouseWheelUpActionPrefChanged_(' +
      'prefs.brave.gestures.mouse_wheel_up_action.value)',
      'onMouseWheelDownActionPrefChanged_(' +
      'prefs.brave.gestures.mouse_wheel_down_action.value)',
    ]
  }

  static get properties() {
    return {
      directionalGestureMappingActionRows_: { type: Array, value: () => [] },
      showDirectionalGestureMappingsResetConfirmDialog_: { type: Boolean, value: false },

      customGestureActionRows_: { type: Array, value: () => [] },
      drawnCustomGestureActionRows_: {
        type: Array, value: () => [defaultActionRow(GestureCommand.UNASSIGNED)]
      },
      hasDrawnCustomGesture_: { type: Boolean, value: false },
      drawnCustomGestureSvgPathData_: { type: String, value: '' },
      showClearCustomGestureConfirmDialog_: { type: Boolean, value: false },

      mouseRockerLeftActionRow_: {
        type: Object, value: () => (
          {
            prefName: GESTURES_MOUSE_ROCKER_LEFT_PREF,
            ...defaultActionRow(33000 /* IDC_BACK */),
            chainedActionRows: []
          })
      },
      mouseRockerRightActionRow_: {
        type: Object, value: () => (
          {
            prefName: GESTURES_MOUSE_ROCKER_RIGHT_PREF,
            ...defaultActionRow(33001 /* IDC_FORWARD */),
            chainedActionRows: []
          })
      },
      showMouseRockerResetConfirmDialog_: { type: Boolean, value: false },

      mouseWheelUpActionRow_: {
        type: Object, value: () => (
          {
            prefName: GESTURES_MOUSE_WHEEL_UP_PREF,
            ...defaultActionRow(34017 /* IDC_SELECT_PREVIOUS_TAB */),
            chainedActionRows: []
          })
      },
      mouseWheelDownActionRow_: {
        type: Object, value: () => (
          {
            prefName: GESTURES_MOUSE_WHEEL_DOWN_PREF,
            ...defaultActionRow(34016 /* IDC_SELECT_NEXT_TAB */),
            chainedActionRows: []
          })
      },
      showMouseWheelResetConfirmDialog_: { type: Boolean, value: false },

      disallowedSiteRows_: { type: Array, value: () => [] },
      showAddDisallowedSiteDialog_: { type: Boolean, value: false },
      addDisallowedSiteInput_: { type: String, value: '' },
      editingDisallowedSiteOrigin_: { type: String, value: null },

      commandOptions_: { type: Array, value: () => [] },
      urlDialogInput_: { type: String, value: '' },
      urlDialogContext_: { type: Object, value: () => null },
      openSettingsPageOptions_: {
        type: Array, value: () => OPEN_SETTINGS_PAGE_OPTIONS
      },
    }
  }

  declare private directionalGestureMappingActionRows_: DirectionalGestureMappingActionRow[]
  declare private showDirectionalGestureMappingsResetConfirmDialog_: boolean

  declare private customGestureActionRows_: CustomGestureActionRow[]

  private isDrawingCustomGesture_ = false
  private drawnCustomGesturePoints_: Point[] = []
  declare private drawnCustomGestureSvgPathData_: string
  declare private hasDrawnCustomGesture_: boolean
  declare private drawnCustomGestureActionRows_: ActionRow[]
  declare private showClearCustomGestureConfirmDialog_: boolean

  declare private mouseRockerLeftActionRow_: MouseRockerActionRow
  declare private mouseRockerRightActionRow_: MouseRockerActionRow
  declare private showMouseRockerResetConfirmDialog_: boolean

  declare private mouseWheelUpActionRow_: MouseWheelActionRow
  declare private mouseWheelDownActionRow_: MouseWheelActionRow
  declare private showMouseWheelResetConfirmDialog_: boolean

  declare private disallowedSiteRows_: DisallowedSiteRow[]
  declare private showAddDisallowedSiteDialog_: boolean
  declare private addDisallowedSiteInput_: string
  declare private editingDisallowedSiteOrigin_: string | null

  declare private commandOptions_: CommandOption[]
  declare private urlDialogInput_: string
  declare private openSettingsPageOptions_:
    Array<{ name: string, value: OpenSettingsPage }>

  // One dialog shared across row types; `row` identifies which row may
  // confirm it, `write` persists the result back to that row.
  declare private urlDialogContext_: {
    row: unknown
    actionRows: ActionRow[]
    targetAction: ActionRow
    write: (actionRows: ActionRow[]) => void
  } | null

  // See `urlDialogContext_`; set on dragstart, read and cleared on drop.
  private actionDragContext_: {
    row: unknown
    actionRows: ActionRow[]
    draggedAction: ActionRow
    write: (actionRows: ActionRow[]) => void
  } | null = null

  // Bound once so `removeListener` in `disconnectedCallback` removes the
  // exact same function reference `connectedCallback` added.
  private boundOnCommandsChanged_ = this.onCommandsChanged_.bind(this)

  override connectedCallback() {
    super.connectedCallback()
    commandsCache.addListener(this.boundOnCommandsChanged_)
  }

  override disconnectedCallback() {
    super.disconnectedCallback()
    commandsCache.removeListener(this.boundOnCommandsChanged_)
  }

  // Sentinels first, in declaration order, since `UNASSIGNED` is the
  // default; then every real browser command alphabetically by name.
  private onCommandsChanged_(commands: { [id: number]: Command }) {
    const sentinelOptions = [
      GestureCommand.UNASSIGNED,
      GestureCommand.NAVIGATE_TO_URL,
      GestureCommand.SCROLL_TO_TOP,
      GestureCommand.SWITCH_TO_TAB,
    ].map((value) => (
      { name: this.i18n(SPECIAL_COMMAND_I18N_KEYS[value]), value }))
    const browserCommandOptions = Object.values(commands)
      .map((command) => ({ name: command.name, value: command.id }))
      .sort((a, b) => a.name.localeCompare(b.name))
    this.commandOptions_ = [...sentinelOptions, ...browserCommandOptions]
  }

  private onDirectionalGestureMappingsPrefChanged_(
    overrides: Record<string, ActionDict[]> | undefined) {
    this.directionalGestureMappingActionRows_ =
      buildDirectionalGestureMappingActionRows(overrides)
  }

  private onDisallowedSitesPrefChanged_(
    disallowedSites: Record<string, boolean> | undefined) {
    this.disallowedSiteRows_ =
      Object.keys(disallowedSites ?? {}).map((origin) => ({ origin }))
  }

  private onCustomGesturesPrefChanged_(
    customGestures: CustomGestureEntry[] | undefined) {
    this.customGestureActionRows_ = buildCustomGestureActionRows(customGestures)
  }

  private buildMouseRockerRow_(
      prefName: string, actions: ActionDict[] | undefined,
    defaultCommand: number): MouseRockerActionRow {
    return { prefName, actionRows: actionRowsFromDicts(actions, defaultCommand) }
  }

  private buildMouseWheelRow_(
    prefName: string, actions: ActionDict[] | undefined,
    defaultCommand: number): MouseWheelActionRow {
    return { prefName, actionRows: actionRowsFromDicts(actions, defaultCommand) }
  }

  private onMouseRockerLeftActionPrefChanged_(
    actions: ActionDict[] | undefined) {
    this.mouseRockerLeftActionRow_ = this.buildMouseRockerRow_(
      GESTURES_MOUSE_ROCKER_LEFT_PREF, actions, 33000 /* IDC_BACK */)
  }

  private onMouseRockerRightActionPrefChanged_(
    actions: ActionDict[] | undefined) {
    this.mouseRockerRightActionRow_ = this.buildMouseRockerRow_(
      GESTURES_MOUSE_ROCKER_RIGHT_PREF, actions, 33001 /* IDC_FORWARD */)
  }

  private onMouseWheelUpActionPrefChanged_(
    actions: ActionDict[] | undefined) {
    this.mouseWheelUpActionRow_ = this.buildMouseWheelRow_(
      GESTURES_MOUSE_WHEEL_UP_PREF, actions, 34017 /* IDC_SELECT_PREVIOUS_TAB */)
  }

  private onMouseWheelDownActionPrefChanged_(
    actions: ActionDict[] | undefined) {
    this.mouseWheelDownActionRow_ = this.buildMouseWheelRow_(
      GESTURES_MOUSE_WHEEL_DOWN_PREF, actions, 34016 /* IDC_SELECT_NEXT_TAB */)
  }

  // The rocker's 2 rows are each their own standalone list pref, so a
  // single writer parameterized by `prefName` covers both.
  private writeMouseRockerActions_(prefName: string, actionRows: ActionRow[]) {
    this.setPrefValue(prefName, toActionDicts(actionRows))
  }

  // Kept separate from `writeMouseRockerActions_` since the two features'
  // prefs and reset behavior are otherwise independent.
  private writeMouseWheelActions_(prefName: string, actionRows: ActionRow[]) {
    this.setPrefValue(prefName, toActionDicts(actionRows))
  }

  private getMouseRockerRow_(prefName: string): MouseRockerActionRow {
    switch (prefName) {
      case GESTURES_MOUSE_ROCKER_LEFT_PREF: return this.mouseRockerLeftActionRow_
      case GESTURES_MOUSE_ROCKER_RIGHT_PREF: return this.mouseRockerRightActionRow_
      default: throw new Error(`Unknown mouse rocker pref: ${prefName}`)
    }
  }

  // See `getMouseRockerRow_`; the wheel's 2 rows (up/down) dispatch the
  // same way.
  private getMouseWheelRow_(prefName: string): MouseWheelActionRow {
    switch (prefName) {
      case GESTURES_MOUSE_WHEEL_UP_PREF: return this.mouseWheelUpActionRow_
      case GESTURES_MOUSE_WHEEL_DOWN_PREF: return this.mouseWheelDownActionRow_
      default: throw new Error(`Unknown mouse wheel pref: ${prefName}`)
    }
  }

  // The rocker and wheel rows share identical dispatch, each container
  // carrying a `data-pref` attribute naming its own pref.
  private getPrefFromDataAttribute_(e: Event): string {
    return (e.currentTarget as HTMLElement).closest<HTMLElement>(
      '[data-pref]')!.dataset['pref']!
  }

  private onMouseRockerCommandChanged_(e: Event) {
    const prefName = this.getPrefFromDataAttribute_(e)
    const row = this.getMouseRockerRow_(prefName)
    const command = Number((e.target as HTMLSelectElement).value)
    this.writeMouseRockerActions_(prefName, [
      { ...row.actionRows[0], command }, ...this.chainedActionRows_(row.actionRows)])
  }

  private onMouseWheelCommandChanged_(e: Event) {
    const prefName = this.getPrefFromDataAttribute_(e)
    const row = this.getMouseWheelRow_(prefName)
    const command = Number((e.target as HTMLSelectElement).value)
    this.writeMouseWheelActions_(prefName, [
      { ...row.actionRows[0], command }, ...this.chainedActionRows_(row.actionRows)])
  }

  private onMouseRockerOpenSettingsPageChanged_(e: Event) {
    const prefName = this.getPrefFromDataAttribute_(e)
    const row = this.getMouseRockerRow_(prefName)
    const openSettingsPage = Number((e.target as HTMLSelectElement).value)
    this.writeMouseRockerActions_(prefName, [
      { ...row.actionRows[0], openSettingsPage }, ...this.chainedActionRows_(row.actionRows)])
  }

  private onMouseWheelOpenSettingsPageChanged_(e: Event) {
    const prefName = this.getPrefFromDataAttribute_(e)
    const row = this.getMouseWheelRow_(prefName)
    const openSettingsPage = Number((e.target as HTMLSelectElement).value)
    this.writeMouseWheelActions_(prefName, [
      { ...row.actionRows[0], openSettingsPage }, ...this.chainedActionRows_(row.actionRows)])
  }

  private onMouseWheelModifierButtonChanged_(e: Event) {
    this.setPrefValue(
      GESTURES_MOUSE_WHEEL_MODIFIER_BUTTON_PREF,
      Number((e.target as HTMLSelectElement).value))
  }

  private onMouseWheelSensitivityChanged_(e: Event) {
    const value = Number((e.target as HTMLInputElement).value)
    const snapped = this.snapToDefault_(
      value, MOUSE_WHEEL_SENSITIVITY_SNAP_THRESHOLD_IN_PIXELS,
      DEFAULT_MOUSE_WHEEL_SENSITIVITY_IN_PIXELS)
    this.setPrefValue(GESTURES_MOUSE_WHEEL_SENSITIVITY_IN_PIXELS_PREF, snapped)
  }

  private onEditMouseRockerUrlClick_(e: Event) {
    const prefName = this.getPrefFromDataAttribute_(e)
    const row = this.getMouseRockerRow_(prefName)
    this.urlDialogContext_ = {
      row, actionRows: row.actionRows, targetAction: row.actionRows[0],
      write: (actionRows) => this.writeMouseRockerActions_(prefName, actionRows),
    }
    this.urlDialogInput_ = row.actionRows[0].textParam
  }

  private onEditMouseWheelUrlClick_(e: Event) {
    const prefName = this.getPrefFromDataAttribute_(e)
    const row = this.getMouseWheelRow_(prefName)
    this.urlDialogContext_ = {
      row, actionRows: row.actionRows, targetAction: row.actionRows[0],
      write: (actionRows) => this.writeMouseWheelActions_(prefName, actionRows),
    }
    this.urlDialogInput_ = row.actionRows[0].textParam
  }

  private onAddMouseRockerChainedActionClick_(e: Event) {
    // Rebuilding the row below replaces this row's DOM; blur first so focus
    // settles on the body instead of wherever it lands afterward.
    ;(e.target as HTMLElement).blur()
    const prefName = this.getPrefFromDataAttribute_(e)
    const row = this.getMouseRockerRow_(prefName)
    this.writeMouseRockerActions_(prefName, [
      ...row.actionRows,
      defaultActionRow(GestureCommand.UNASSIGNED),
    ])
  }

  private onAddMouseWheelChainedActionClick_(e: Event) {
    ; (e.target as HTMLElement).blur()
    const prefName = this.getPrefFromDataAttribute_(e)
    const row = this.getMouseWheelRow_(prefName)
    this.writeMouseWheelActions_(prefName, [
      ...row.actionRows,
      defaultActionRow(GestureCommand.UNASSIGNED),
    ])
  }

  // Only shown when there's at least one chained action, so there's always
  // a next action to promote into the primary slot this one vacates.
  private onRemoveMouseRockerPrimaryActionClick_(e: Event) {
    const prefName = this.getPrefFromDataAttribute_(e)
    const row = this.getMouseRockerRow_(prefName)
    this.writeMouseRockerActions_(prefName, this.chainedActionRows_(row.actionRows))
  }

  // See `onRemoveMouseRockerPrimaryActionClick_`.
  private onRemoveMouseWheelPrimaryActionClick_(e: Event) {
    const prefName = this.getPrefFromDataAttribute_(e)
    const row = this.getMouseWheelRow_(prefName)
    this.writeMouseWheelActions_(prefName, this.chainedActionRows_(row.actionRows))
  }

  private onMouseRockerChainedActionCommandChanged_(e: Event &
    { model: { chainedAction: ActionRow } }) {
    const prefName = this.getPrefFromDataAttribute_(e)
    const row = this.getMouseRockerRow_(prefName)
    const command = Number((e.target as HTMLSelectElement).value)
    this.writeMouseRockerActions_(
      prefName, row.actionRows.map(
        (action) => action === e.model.chainedAction
          ? { ...action, command }
          : action))
  }

  private onMouseWheelChainedActionCommandChanged_(e: Event &
    { model: { chainedAction: ActionRow } }) {
    const prefName = this.getPrefFromDataAttribute_(e)
    const row = this.getMouseWheelRow_(prefName)
    const command = Number((e.target as HTMLSelectElement).value)
    this.writeMouseWheelActions_(
      prefName, row.actionRows.map(
        (action) => action === e.model.chainedAction
          ? { ...action, command }
          : action))
  }

  private onMouseRockerChainedActionOpenSettingsPageChanged_(e: Event &
  { model: { chainedAction: ActionRow } }) {
    const prefName = this.getPrefFromDataAttribute_(e)
    const row = this.getMouseRockerRow_(prefName)
    const openSettingsPage = Number((e.target as HTMLSelectElement).value)
    this.writeMouseRockerActions_(
      prefName, row.actionRows.map(
        (action) => action === e.model.chainedAction
          ? { ...action, openSettingsPage }
          : action))
  }

  private onMouseWheelChainedActionOpenSettingsPageChanged_(e: Event &
  { model: { chainedAction: ActionRow } }) {
    const prefName = this.getPrefFromDataAttribute_(e)
    const row = this.getMouseWheelRow_(prefName)
    const openSettingsPage = Number((e.target as HTMLSelectElement).value)
    this.writeMouseWheelActions_(
      prefName, row.actionRows.map(
        (action) => action === e.model.chainedAction
          ? { ...action, openSettingsPage }
          : action))
  }

  private onRemoveMouseRockerChainedActionClick_(e: Event &
  { model: { chainedAction: ActionRow } }) {
    const prefName = this.getPrefFromDataAttribute_(e)
    const row = this.getMouseRockerRow_(prefName)
    this.writeMouseRockerActions_(
      prefName,
      row.actionRows.filter(
        (action) => action !== e.model.chainedAction))
  }

  private onRemoveMouseWheelChainedActionClick_(e: Event &
    { model: { chainedAction: ActionRow } }) {
    const prefName = this.getPrefFromDataAttribute_(e)
    const row = this.getMouseWheelRow_(prefName)
    this.writeMouseWheelActions_(
      prefName,
      row.actionRows.filter(
        (action) => action !== e.model.chainedAction))
  }

  private onEditMouseRockerChainedUrlClick_(e: Event &
  { model: { chainedAction: ActionRow } }) {
    const prefName = this.getPrefFromDataAttribute_(e)
    const row = this.getMouseRockerRow_(prefName)
    this.urlDialogContext_ = {
      row, actionRows: row.actionRows, targetAction: e.model.chainedAction,
      write: (actionRows) => this.writeMouseRockerActions_(prefName, actionRows),
    }
    this.urlDialogInput_ = e.model.chainedAction.textParam
  }

  private onEditMouseWheelChainedUrlClick_(e: Event &
  { model: { chainedAction: ActionRow } }) {
    const prefName = this.getPrefFromDataAttribute_(e)
    const row = this.getMouseWheelRow_(prefName)
    this.urlDialogContext_ = {
      row, actionRows: row.actionRows, targetAction: e.model.chainedAction,
      write: (actionRows) => this.writeMouseWheelActions_(prefName, actionRows),
    }
    this.urlDialogInput_ = e.model.chainedAction.textParam
  }

  private onMouseRockerPrimaryActionDragStart_(e: DragEvent) {
    const prefName = this.getPrefFromDataAttribute_(e)
    const row = this.getMouseRockerRow_(prefName)
    this.actionDragContext_ = {
      row, actionRows: row.actionRows, draggedAction: row.actionRows[0],
      write: (actionRows) => this.writeMouseRockerActions_(prefName, actionRows),
    }
    e.dataTransfer!.effectAllowed = 'move'
    e.dataTransfer!.setData('text/plain', '')
  }

  private onMouseWheelPrimaryActionDragStart_(e: DragEvent) {
    const prefName = this.getPrefFromDataAttribute_(e)
    const row = this.getMouseWheelRow_(prefName)
    this.actionDragContext_ = {
      row, actionRows: row.actionRows, draggedAction: row.actionRows[0],
      write: (actionRows) => this.writeMouseWheelActions_(prefName, actionRows),
    }
    e.dataTransfer!.effectAllowed = 'move'
    e.dataTransfer!.setData('text/plain', '')
  }

  private onMouseRockerPrimaryActionDrop_(e: DragEvent) {
    e.preventDefault()
    const prefName = this.getPrefFromDataAttribute_(e)
    const row = this.getMouseRockerRow_(prefName)
    const context = this.actionDragContext_
    this.actionDragContext_ = null
    if (context?.row !== row) {
      return
    }
    context.write(moveAction(
      context.actionRows, context.draggedAction, row.actionRows[0]))
  }

  private onMouseWheelPrimaryActionDrop_(e: DragEvent) {
    e.preventDefault()
    const prefName = this.getPrefFromDataAttribute_(e)
    const row = this.getMouseWheelRow_(prefName)
    const context = this.actionDragContext_
    this.actionDragContext_ = null
    if (context?.row !== row) {
      return
    }
    context.write(moveAction(
      context.actionRows, context.draggedAction, row.actionRows[0]))
  }

  private onMouseRockerChainedActionDragStart_(e: DragEvent &
    { model: { chainedAction: ActionRow } }) {
    const prefName = this.getPrefFromDataAttribute_(e)
    const row = this.getMouseRockerRow_(prefName)
    this.actionDragContext_ = {
      row, actionRows: row.actionRows, draggedAction: e.model.chainedAction,
      write: (actionRows) => this.writeMouseRockerActions_(prefName, actionRows),
    }
    e.dataTransfer!.effectAllowed = 'move'
    e.dataTransfer!.setData('text/plain', '')
  }

  private onMouseWheelChainedActionDragStart_(e: DragEvent &
  { model: { chainedAction: ActionRow } }) {
    const prefName = this.getPrefFromDataAttribute_(e)
    const row = this.getMouseWheelRow_(prefName)
    this.actionDragContext_ = {
      row, actionRows: row.actionRows, draggedAction: e.model.chainedAction,
      write: (actionRows) => this.writeMouseWheelActions_(prefName, actionRows),
    }
    e.dataTransfer!.effectAllowed = 'move'
    e.dataTransfer!.setData('text/plain', '')
  }

  private onMouseRockerChainedActionDrop_(e: DragEvent &
    { model: { chainedAction: ActionRow } }) {
    e.preventDefault()
    const row = this.getMouseRockerRow_(this.getPrefFromDataAttribute_(e))
    const context = this.actionDragContext_
    this.actionDragContext_ = null
    if (context?.row !== row) {
      return
    }
    context.write(moveAction(
      context.actionRows, context.draggedAction, e.model.chainedAction))
  }

  private onMouseWheelChainedActionDrop_(e: DragEvent &
  { model: { chainedAction: ActionRow } }) {
    e.preventDefault()
    const row = this.getMouseWheelRow_(this.getPrefFromDataAttribute_(e))
    const context = this.actionDragContext_
    this.actionDragContext_ = null
    if (context?.row !== row) {
      return
    }
    context.write(moveAction(
      context.actionRows, context.draggedAction, e.model.chainedAction))
  }

  // True for any command whose destination/pattern is edited via the
  // shared URL/pattern dialog (see `urlDialogContext_`'s own comment).
  private hasTextParam_(command: number): boolean {
    return command === GestureCommand.NAVIGATE_TO_URL ||
      command === GestureCommand.SWITCH_TO_TAB
  }

  private isOpenSettingsCommand_(command: number): boolean {
    return command === IDC_OPTIONS
  }

  private getCommandSelectClass_(
    command: number, textParam: string): string {
    const classes = ['md-select']
    if (command === GestureCommand.UNASSIGNED) {
      classes.push('command-unassigned')
    }
    if (this.isActionIncomplete_(command, textParam)) {
      classes.push('action-incomplete')
    }
    return classes.join(' ')
  }

  // Only `textParam` has an "unset" state; every other param type defaults
  // to a value that is already valid on its own.
  private isActionIncomplete_(
    command: number, textParam: string): boolean {
    return command === GestureCommand.UNASSIGNED ||
      (this.hasTextParam_(command) && !textParam)
  }

  private getUrlAttachClass_(textParam: string): string {
    return textParam
      ? 'action-param param-attach'
      : 'action-param param-attach action-incomplete'
  }

  // Native `<option>` reflects the `<select>`'s value via its own
  // `selected` attribute, not a one-way binding.
  private optionMatchesCommand_(
    optionValue: number, command: number): boolean {
    return optionValue === command
  }

  // Template bindings cannot nest a method call inside another method
  // call's argument list, so these flatten `primaryActionRow_(actionRows)`
  // into a single call for markup that needs the primary action's fields.
  private primaryCommandSelectClass_(actionRows: ActionRow[]): string {
    const primary = this.primaryActionRow_(actionRows)
    return this.getCommandSelectClass_(primary.command, primary.textParam)
  }

  private optionMatchesPrimaryCommand_(
    optionValue: number, actionRows: ActionRow[]): boolean {
    return this.optionMatchesCommand_(
      optionValue, this.primaryActionRow_(actionRows).command)
  }

  private isPrimaryOpenSettingsCommand_(actionRows: ActionRow[]): boolean {
    return this.isOpenSettingsCommand_(
      this.primaryActionRow_(actionRows).command)
  }

  private optionMatchesPrimaryOpenSettingsPage_(
    optionValue: number, actionRows: ActionRow[]): boolean {
    return this.optionMatchesCommand_(
      optionValue, this.primaryActionRow_(actionRows).openSettingsPage)
  }

  private primaryUrlAttachClass_(actionRows: ActionRow[]): string {
    return this.getUrlAttachClass_(this.primaryActionRow_(actionRows).textParam)
  }

  private primaryHasTextParam_(actionRows: ActionRow[]): boolean {
    return this.hasTextParam_(this.primaryActionRow_(actionRows).command)
  }

  private primaryUrlButtonLabel_(actionRows: ActionRow[]): string {
    const primary = this.primaryActionRow_(actionRows)
    return this.getUrlButtonLabel_(primary.command, primary.textParam)
  }

  private onCommandChanged_(
    e: DomRepeatEvent<DirectionalGestureMappingActionRow>) {
    const row = e.model.item
    this.reassignCommand_(
      Number((e.target as HTMLSelectElement).value),
      { directionalGestureMappingKey: row.key })
  }

  private onCustomGestureCommandChanged_(
    e: DomRepeatEvent<CustomGestureActionRow>) {
    const row = e.model.item
    this.reassignCommand_(
      Number((e.target as HTMLSelectElement).value),
      { customGestureId: row.id })
  }

  private onEditDirectionalGestureMappingUrlClick_(e: DomRepeatEvent<DirectionalGestureMappingActionRow>) {
    const row = e.model.item
    this.urlDialogContext_ = {
      row, actionRows: row.actionRows, targetAction: row.actionRows[0],
      write: (actionRows) =>
        this.writeDirectionalGestureMappingActions_(row.key, actionRows),
    }
    this.urlDialogInput_ = row.actionRows[0].textParam
  }

  private onEditCustomGestureUrlClick_(e: DomRepeatEvent<CustomGestureActionRow>) {
    const row = e.model.item
    this.urlDialogContext_ = {
      row, actionRows: row.actionRows, targetAction: row.actionRows[0],
      write: (actionRows) => this.writeCustomGestureActions_(row.id, actionRows),
    }
    this.urlDialogInput_ = row.actionRows[0].textParam
  }

  // Only shown when there's at least one chained action, so there's always
  // a next action to promote into the primary slot this one vacates.
  private onRemoveDirectionalGestureMappingPrimaryActionClick_(e: DomRepeatEvent<DirectionalGestureMappingActionRow>) {
    const row = e.model.item
    this.writeDirectionalGestureMappingActions_(row.key, this.chainedActionRows_(row.actionRows))
  }

  private onRemoveCustomGesturePrimaryActionClick_(
    e: DomRepeatEvent<CustomGestureActionRow>) {
    const row = e.model.item
    this.writeCustomGestureActions_(row.id, this.chainedActionRows_(row.actionRows))
  }

  private onUrlDialogInputChanged_(e: Event) {
    this.urlDialogInput_ = (e.target as HTMLInputElement).value
  }

  private onUrlDialogClose_() {
    this.urlDialogContext_ = null
  }

  private onConfirmUrlDialogClick_() {
    const context = this.urlDialogContext_
    if (!context) {
      return
    }
    // A Switch to Tab pattern is matched literally, not navigated to, so it
    // does not get an `https://` scheme prepended the way a URL does.
    const input = this.urlDialogInput_.trim()
    const url = context.targetAction.command === GestureCommand.SWITCH_TO_TAB
      ? input : withUrlScheme(input)
    context.write(context.actionRows.map((action) =>
      action === context.targetAction ? { ...action, textParam: url } : action))
    this.urlDialogContext_ = null
  }

  // A pattern is shown as stored; a URL has its scheme stripped for
  // display only, since it is always http/https.
  private getUrlButtonLabel_(
    command: number, textParam: string): string {
    if (textParam) {
      return command === GestureCommand.SWITCH_TO_TAB
        ? textParam : textParam.replace(/^https?:\/\//, '')
    }
    return this.i18n(command === GestureCommand.SWITCH_TO_TAB
      ? 'braveGesturesSetPattern' : 'braveGesturesSetUrl')
  }

  private urlDialogTitle_(): string {
    return this.i18n(
      this.urlDialogContext_?.targetAction.command === GestureCommand.SWITCH_TO_TAB
        ? 'braveGesturesSpecialCommandSwitchToTab'
        : 'braveGesturesSpecialCommandNavigateToUrl')
  }

  private urlDialogPlaceholder_(): string {
    return this.i18n(
      this.urlDialogContext_?.targetAction.command === GestureCommand.SWITCH_TO_TAB
        ? 'braveGesturesSwitchToTabPlaceholder'
        : 'braveGesturesNavigateToUrlPlaceholder')
  }

  private writeDirectionalGestureMappingActions_(
    key: string, actionRows: ActionRow[]) {
    this.setPrefDictEntry(
      GESTURES_DIRECTIONAL_GESTURE_MAPPINGS_PREF, key,
      toActionDicts(actionRows))
  }

  private existingCustomGestures_(): CustomGestureEntry[] {
    return this.getPref<CustomGestureEntry[]>(
      GESTURES_CUSTOM_GESTURES_PREF).value ?? []
  }

  private writeCustomGestureActions_(id: string, actionRows: ActionRow[]) {
    const existingCustomGestures = this.existingCustomGestures_()
    this.setPrefValue(
      GESTURES_CUSTOM_GESTURES_PREF,
      existingCustomGestures.map((customGesture) => customGesture.id === id
        ? { ...customGesture, actions: toActionDicts(actionRows) }
        : customGesture))
  }

  private onAddDirectionalGestureMappingChainedActionClick_(e: DomRepeatEvent<DirectionalGestureMappingActionRow>) {
    // Rebuilding the row below replaces this button's DOM, so blur it
    // first or focus would land wherever the replacement happens to be.
    ;(e.target as HTMLElement).blur()
    const row = e.model.item
    this.writeDirectionalGestureMappingActions_(row.key, [
      ...row.actionRows,
      defaultActionRow(GestureCommand.UNASSIGNED),
    ])
  }

  private onRemoveDirectionalGestureMappingChainedActionClick_(e: Event &
    { model: { chainedAction: ActionRow,
      parentModel: { item: DirectionalGestureMappingActionRow }
    }
    }) {
    const row = e.model.parentModel.item
    this.writeDirectionalGestureMappingActions_(
      row.key,
      row.actionRows.filter(
        (action) => action !== e.model.chainedAction))
  }

  private onDirectionalGestureMappingChainedActionCommandChanged_(e: Event &
    { model: { chainedAction: ActionRow,
      parentModel: { item: DirectionalGestureMappingActionRow }
    }
    }) {
    const row = e.model.parentModel.item
    const command = Number((e.target as HTMLSelectElement).value)
    this.writeDirectionalGestureMappingActions_(
      row.key, row.actionRows.map(
        (action) => action === e.model.chainedAction
          ? { ...action, command }
          : action))
  }

  private onDirectionalGestureMappingOpenSettingsPageChanged_(e: Event &
  { model: { item: DirectionalGestureMappingActionRow } }) {
    const row = e.model.item
    const openSettingsPage = Number((e.target as HTMLSelectElement).value)
    this.writeDirectionalGestureMappingActions_(row.key, [
      { ...row.actionRows[0], openSettingsPage }, ...this.chainedActionRows_(row.actionRows)])
  }

  private onDirectionalGestureMappingChainedActionOpenSettingsPageChanged_(e: Event &
  {
    model: {
      chainedAction: ActionRow,
      parentModel: { item: DirectionalGestureMappingActionRow }
    }
  }) {
    const row = e.model.parentModel.item
    const openSettingsPage = Number((e.target as HTMLSelectElement).value)
    this.writeDirectionalGestureMappingActions_(
      row.key, row.actionRows.map(
        (action) => action === e.model.chainedAction
          ? { ...action, openSettingsPage }
          : action))
  }

  private onEditDirectionalGestureMappingChainedUrlClick_(e: Event &
    { model: { chainedAction: ActionRow,
      parentModel: { item: DirectionalGestureMappingActionRow }
    }
    }) {
    const row = e.model.parentModel.item
    this.urlDialogContext_ = {
      row, actionRows: row.actionRows, targetAction: e.model.chainedAction,
      write: (actionRows) =>
        this.writeDirectionalGestureMappingActions_(row.key, actionRows),
    }
    this.urlDialogInput_ = e.model.chainedAction.textParam
  }

  // Allows a drop; without this, the browser rejects it. Do not add
  // mid-drag reordering here; it has twice crashed the browser process.
  private onActionDragOver_(e: DragEvent) {
    e.preventDefault()
    if (e.dataTransfer) {
      e.dataTransfer.dropEffect = 'move'
    }
  }

  // A no-op if a drop already cleared `actionDragContext_`; otherwise
  // clears a drag that ended outside any valid target.
  private onActionDragEnd_() {
    this.actionDragContext_ = null
  }

  // Template helpers so markup can split a row's `actionRows` (index 0
  // primary) without a dedicated field for each half.
  private primaryActionRow_(actionRows: ActionRow[]): ActionRow {
    return actionRows[0]
  }

  private chainedActionRows_(actionRows: ActionRow[]): ActionRow[] {
    return actionRows.slice(1)
  }

  // Flattens a method call plus property access into one call, since
  // bindings cannot chain the two.
  private hasChainedActions_(actionRows: ActionRow[]): boolean {
    return this.chainedActionRows_(actionRows).length > 0
  }

  private onDirectionalGestureMappingChainedActionDragStart_(e: DragEvent &
    { model: { chainedAction: ActionRow,
      parentModel: { item: DirectionalGestureMappingActionRow }
    }
    }) {
    const row = e.model.parentModel.item
    this.actionDragContext_ = {
      row, actionRows: row.actionRows, draggedAction: e.model.chainedAction,
      write: (actionRows) =>
        this.writeDirectionalGestureMappingActions_(row.key, actionRows),
    }
    e.dataTransfer!.effectAllowed = 'move'
    // Some drag implementations only treat the drag as valid, and fire
    // `drop`, once `dataTransfer` actually carries data.
    e.dataTransfer!.setData('text/plain', '')
  }

  private onDirectionalGestureMappingChainedActionDrop_(e: DragEvent &
    { model: { chainedAction: ActionRow,
      parentModel: { item: DirectionalGestureMappingActionRow }
    }
    }) {
    e.preventDefault()
    const row = e.model.parentModel.item
    const context = this.actionDragContext_
    this.actionDragContext_ = null
    if (context?.row !== row) {
      return
    }
    context.write(moveAction(
      context.actionRows, context.draggedAction, e.model.chainedAction))
  }

  private onDirectionalGestureMappingPrimaryActionDragStart_(
    e: DragEvent & { model: { item: DirectionalGestureMappingActionRow } }) {
    const row = e.model.item
    this.actionDragContext_ = {
      row, actionRows: row.actionRows, draggedAction: row.actionRows[0],
      write: (actionRows) =>
        this.writeDirectionalGestureMappingActions_(row.key, actionRows),
    }
    e.dataTransfer!.effectAllowed = 'move'
    e.dataTransfer!.setData('text/plain', '')
  }

  private onDirectionalGestureMappingPrimaryActionDrop_(
    e: DragEvent & { model: { item: DirectionalGestureMappingActionRow } }) {
    e.preventDefault()
    const row = e.model.item
    const context = this.actionDragContext_
    this.actionDragContext_ = null
    if (context?.row !== row) {
      return
    }
    context.write(moveAction(
      context.actionRows, context.draggedAction, row.actionRows[0]))
  }

  private onAddCustomGestureChainedActionClick_(
    e: DomRepeatEvent<CustomGestureActionRow>) {
    // Rebuilding the row below replaces this button's DOM, so blur it
    // first or focus would land wherever the replacement happens to be.
    ;(e.target as HTMLElement).blur()
    const row = e.model.item
    this.writeCustomGestureActions_(
      row.id, [
      ...row.actionRows,
      defaultActionRow(GestureCommand.UNASSIGNED),
      ])
  }

  private onRemoveCustomGestureChainedActionClick_(e: Event &
    { model: { chainedAction: ActionRow,
      parentModel: { item: CustomGestureActionRow }
    }
    }) {
    const row = e.model.parentModel.item
    this.writeCustomGestureActions_(
      row.id,
      row.actionRows.filter(
        (action) => action !== e.model.chainedAction))
  }

  private onCustomGestureChainedActionCommandChanged_(e: Event &
    { model: { chainedAction: ActionRow,
      parentModel: { item: CustomGestureActionRow }
    }
    }) {
    const row = e.model.parentModel.item
    const command = Number((e.target as HTMLSelectElement).value)
    this.writeCustomGestureActions_(
      row.id, row.actionRows.map(
        (action) => action === e.model.chainedAction
          ? { ...action, command }
          : action))
  }

  private onCustomGestureOpenSettingsPageChanged_(e: Event &
  { model: { item: CustomGestureActionRow } }) {
    const row = e.model.item
    const openSettingsPage = Number((e.target as HTMLSelectElement).value)
    this.writeCustomGestureActions_(row.id, [
      { ...row.actionRows[0], openSettingsPage }, ...this.chainedActionRows_(row.actionRows)])
  }

  private onCustomGestureChainedActionOpenSettingsPageChanged_(e: Event &
  {
    model: {
      chainedAction: ActionRow,
      parentModel: { item: CustomGestureActionRow }
    }
  }) {
    const row = e.model.parentModel.item
    const openSettingsPage = Number((e.target as HTMLSelectElement).value)
    this.writeCustomGestureActions_(
      row.id, row.actionRows.map(
        (action) => action === e.model.chainedAction
          ? { ...action, openSettingsPage }
          : action))
  }

  private onEditCustomGestureChainedUrlClick_(e: Event &
    { model: { chainedAction: ActionRow,
      parentModel: { item: CustomGestureActionRow }
    }
    }) {
    const row = e.model.parentModel.item
    this.urlDialogContext_ = {
      row, actionRows: row.actionRows, targetAction: e.model.chainedAction,
      write: (actionRows) => this.writeCustomGestureActions_(row.id, actionRows),
    }
    this.urlDialogInput_ = e.model.chainedAction.textParam
  }

  private onCustomGestureChainedActionDragStart_(e: DragEvent &
    { model: { chainedAction: ActionRow,
      parentModel: { item: CustomGestureActionRow }
    }
    }) {
    const row = e.model.parentModel.item
    this.actionDragContext_ = {
      row, actionRows: row.actionRows, draggedAction: e.model.chainedAction,
      write: (actionRows) => this.writeCustomGestureActions_(row.id, actionRows),
    }
    e.dataTransfer!.effectAllowed = 'move'
    // Some drag implementations only treat the drag as valid, and fire
    // `drop`, once `dataTransfer` actually carries data.
    e.dataTransfer!.setData('text/plain', '')
  }

  private onCustomGestureChainedActionDrop_(e: DragEvent &
    { model: { chainedAction: ActionRow,
      parentModel: { item: CustomGestureActionRow }
    }
    }) {
    e.preventDefault()
    const row = e.model.parentModel.item
    const context = this.actionDragContext_
    this.actionDragContext_ = null
    if (context?.row !== row) {
      return
    }
    context.write(moveAction(
      context.actionRows, context.draggedAction, e.model.chainedAction))
  }

  private onCustomGesturePrimaryActionDragStart_(
    e: DragEvent & { model: { item: CustomGestureActionRow } }) {
    const row = e.model.item
    this.actionDragContext_ = {
      row, actionRows: row.actionRows, draggedAction: row.actionRows[0],
      write: (actionRows) => this.writeCustomGestureActions_(row.id, actionRows),
    }
    e.dataTransfer!.effectAllowed = 'move'
    e.dataTransfer!.setData('text/plain', '')
  }

  private onCustomGesturePrimaryActionDrop_(
    e: DragEvent & { model: { item: CustomGestureActionRow } }) {
    e.preventDefault()
    const row = e.model.item
    const context = this.actionDragContext_
    this.actionDragContext_ = null
    if (context?.row !== row) {
      return
    }
    context.write(moveAction(
      context.actionRows, context.draggedAction, row.actionRows[0]))
  }

  // A gesture may share its command with another; the conflict is flagged in
  // the UI rather than cleared or blocked here.
  private reassignCommand_(
    command: number,
    gestureTarget:
        { directionalGestureMappingKey?: string, customGestureId?: string }) {
    if (gestureTarget.directionalGestureMappingKey !== undefined) {
      const row = this.directionalGestureMappingActionRows_.find(
        (mappingRow) => mappingRow.key === gestureTarget.directionalGestureMappingKey)
      this.writeDirectionalGestureMappingActions_(
        gestureTarget.directionalGestureMappingKey,
        applyCommandToActionRows(command, row?.actionRows))
    }
    if (gestureTarget.customGestureId !== undefined) {
      const row = this.customGestureActionRows_.find(
        (customGestureRow) => customGestureRow.id === gestureTarget.customGestureId)
      this.writeCustomGestureActions_(
        gestureTarget.customGestureId,
        applyCommandToActionRows(command, row?.actionRows))
    }
  }

  // Both action-row arrays are explicit, otherwise-unused parameters so
  // this recomputes when either changes, not just `row`.
  private directionalGestureMappingRowConflictClass_(
    row: DirectionalGestureMappingActionRow, _directionalGestureMappingRows: DirectionalGestureMappingActionRow[],
    _customGestureRows: CustomGestureActionRow[]): string {
    const { keys, customGestureIds } = findConflictingGestures(
      rowCommandSequence(row), { directionalGestureMappingKey: row.key },
      this.directionalGestureMappingActionRows_, this.customGestureActionRows_)
    const hasConflict = keys.length > 0 || customGestureIds.length > 0 ||
      this.anyCustomGestureMatchesDirectionalGestureKey_(row.key)
    return hasConflict ? 'command-conflict' : ''
  }

  private anyCustomGestureMatchesDirectionalGestureKey_(key: string): boolean {
    return anyCustomGestureMatchesDirectionalGestureKey(
      key, this.existingCustomGestures_(), this.minStrokePixels_(),
      MAX_DIRECTIONAL_STROKES, this.directionalGestureMappingActionRows_)
  }

  // Both action-row arrays are explicit, otherwise-unused parameters so
  // this recomputes when either changes, not just `row`.
  private customGestureRowConflictClass_(
    row: CustomGestureActionRow, _directionalGestureMappingRows: DirectionalGestureMappingActionRow[],
    _customGestureRows: CustomGestureActionRow[]): string {
    const { keys, customGestureIds } = findConflictingGestures(
      rowCommandSequence(row), { customGestureId: row.id },
      this.directionalGestureMappingActionRows_, this.customGestureActionRows_)
    const hasConflict = keys.length > 0 || customGestureIds.length > 0 ||
      this.customGestureShapeConflictDescription_(row.id) !== ''
    return hasConflict ? 'command-conflict' : ''
  }

  private customGestureShapeConflictDescription_(id: string): string {
    return customGestureShapeConflictDescription(
      id, this.existingCustomGestures_(), this.minStrokePixels_(),
      MAX_DIRECTIONAL_STROKES, this.directionalGestureMappingActionRows_,
      this.i18n.bind(this))
  }

  private minStrokePixels_(): number {
    return this.getPref<number>(
      GESTURES_SENSITIVITY_IN_PIXELS_PREF).value ?? DEFAULT_SENSITIVITY_IN_PIXELS
  }

  private directionalGestureMappingConflictDescription_(
    row: DirectionalGestureMappingActionRow, _directionalGestureMappingRows: DirectionalGestureMappingActionRow[],
    _customGestureRows: CustomGestureActionRow[]): string {
    const { keys, customGestureIds } = findConflictingGestures(
      rowCommandSequence(row), { directionalGestureMappingKey: row.key },
      this.directionalGestureMappingActionRows_, this.customGestureActionRows_)
    const commandConflict = conflictDescription(keys, customGestureIds, this.i18n.bind(this))
    if (!this.anyCustomGestureMatchesDirectionalGestureKey_(row.key)) {
      return commandConflict
    }
    const shapeConflict = this.i18n('braveGesturesShapeConflictCustomSublabel')
    return commandConflict ? `${commandConflict}. ${shapeConflict}` : shapeConflict
  }

  private customGestureConflictDescription_(
    row: CustomGestureActionRow, _directionalGestureMappingRows: DirectionalGestureMappingActionRow[],
    _customGestureRows: CustomGestureActionRow[]): string {
    const { keys, customGestureIds } = findConflictingGestures(
      rowCommandSequence(row), { customGestureId: row.id },
      this.directionalGestureMappingActionRows_, this.customGestureActionRows_)
    const commandConflict = conflictDescription(keys, customGestureIds, this.i18n.bind(this))
    const shapeConflict = this.customGestureShapeConflictDescription_(row.id)
    if (!shapeConflict) {
      return commandConflict
    }
    return commandConflict ? `${commandConflict}. ${shapeConflict}` : shapeConflict
  }

  private onResetDirectionalGestureMappingsClick_() {
    this.showDirectionalGestureMappingsResetConfirmDialog_ = true
  }

  private onDirectionalGestureMappingsResetDialogClose_() {
    this.showDirectionalGestureMappingsResetConfirmDialog_ = false
  }

  private onConfirmDirectionalGestureMappingsResetClick_() {
    for (const row of this.directionalGestureMappingActionRows_) {
      this.deletePrefDictEntry(GESTURES_DIRECTIONAL_GESTURE_MAPPINGS_PREF, row.key)
    }
    this.showDirectionalGestureMappingsResetConfirmDialog_ = false
  }

  private onResetMouseRockerClick_() {
    this.showMouseRockerResetConfirmDialog_ = true
  }

  private onMouseRockerResetDialogClose_() {
    this.showMouseRockerResetConfirmDialog_ = false
  }

  // Each row is its own standalone pref, so resetting writes each one
  // back to its single-action default.
  private onConfirmMouseRockerResetClick_() {
    this.writeMouseRockerActions_(
      GESTURES_MOUSE_ROCKER_LEFT_PREF, [defaultActionRow(33000 /* IDC_BACK */)])
    this.writeMouseRockerActions_(
      GESTURES_MOUSE_ROCKER_RIGHT_PREF, [defaultActionRow(33001 /* IDC_FORWARD */)])
    this.showMouseRockerResetConfirmDialog_ = false
  }

  private onResetMouseWheelClick_() {
    this.showMouseWheelResetConfirmDialog_ = true
  }

  private onMouseWheelResetDialogClose_() {
    this.showMouseWheelResetConfirmDialog_ = false
  }

  // Also resets the modifier button and sensitivity, since those are
  // themselves configurable for the wheel, unlike the rocker's fixed sides.
  private onConfirmMouseWheelResetClick_() {
    this.writeMouseWheelActions_(
      GESTURES_MOUSE_WHEEL_UP_PREF,
      [defaultActionRow(34017 /* IDC_SELECT_PREVIOUS_TAB */)])
    this.writeMouseWheelActions_(
      GESTURES_MOUSE_WHEEL_DOWN_PREF,
      [defaultActionRow(34016 /* IDC_SELECT_NEXT_TAB */)])
    this.setPrefValue(
      GESTURES_MOUSE_WHEEL_MODIFIER_BUTTON_PREF, DEFAULT_MOUSE_WHEEL_MODIFIER_BUTTON)
    this.setPrefValue(
      GESTURES_MOUSE_WHEEL_SENSITIVITY_IN_PIXELS_PREF,
      DEFAULT_MOUSE_WHEEL_SENSITIVITY_IN_PIXELS)
    this.showMouseWheelResetConfirmDialog_ = false
  }

  private onAddDisallowedSiteClick_() {
    this.editingDisallowedSiteOrigin_ = null
    this.addDisallowedSiteInput_ = ''
    this.showAddDisallowedSiteDialog_ = true
  }

  private onEditDisallowedSiteClick_(e: DomRepeatEvent<DisallowedSiteRow>) {
    const row = e.model.item
    this.editingDisallowedSiteOrigin_ = row.origin
    this.addDisallowedSiteInput_ = row.origin
    this.showAddDisallowedSiteDialog_ = true
  }

  private getAddDisallowedSiteDialogTitle_(editingSiteOrigin: string | null): string {
    return editingSiteOrigin !== null
      ? this.i18n('braveGesturesEditSite')
      : this.i18n('braveGesturesAddSite')
  }

  private onAddSiteDialogClose_() {
    this.showAddDisallowedSiteDialog_ = false
  }

  private onAddSiteInputChanged_(e: Event) {
    this.addDisallowedSiteInput_ = (e.target as HTMLInputElement).value
  }

  private onConfirmAddSiteClick_() {
    // Matched by bare hostname, so a pasted full URL is normalized down
    // to just its hostname.
    const input = this.addDisallowedSiteInput_.trim()
    if (!input) {
      return
    }
    let host = input
    try {
      host = new URL(
        host.includes('://') ? host : `https://${host}`).hostname
    } catch {
      // Not a parseable URL; fall back to using the trimmed input as-is.
    }
    if (this.editingDisallowedSiteOrigin_ !== null &&
        this.editingDisallowedSiteOrigin_ !== host) {
      this.deletePrefDictEntry(
        GESTURES_DISALLOWED_SITES_PREF, this.editingDisallowedSiteOrigin_)
    }
    this.setPrefDictEntry(GESTURES_DISALLOWED_SITES_PREF, host, true)
    this.showAddDisallowedSiteDialog_ = false
  }

  private onRemoveDisallowedSiteClick_(e: DomRepeatEvent<DisallowedSiteRow>) {
    const row = e.model.item
    this.deletePrefDictEntry(GESTURES_DISALLOWED_SITES_PREF, row.origin)
  }

  private onSensitivityChanged_(e: Event) {
    const value = Number((e.target as HTMLInputElement).value)
    const snapped = this.snapToDefault_(
      value, SENSITIVITY_SNAP_THRESHOLD_IN_PIXELS, DEFAULT_SENSITIVITY_IN_PIXELS)
    this.setPrefValue(GESTURES_SENSITIVITY_IN_PIXELS_PREF, snapped)
  }

  // Lands exactly on the default when released within `threshold` of it, so
  // it is easy to return to the out-of-the-box value.
  private snapToDefault_(
    value: number, threshold: number, defaultValue: number): number {
    return Math.abs(value - defaultValue) <= threshold ? defaultValue : value
  }

  private onDrawnCustomGestureCommandChanged_(e: Event) {
    const [primaryActionRow, ...chainedActionRows] = this.drawnCustomGestureActionRows_
    this.drawnCustomGestureActionRows_ = [
      { ...primaryActionRow, command: Number((e.target as HTMLSelectElement).value) },
      ...chainedActionRows,
    ]
  }

  private onEditDrawnCustomGestureUrlClick_() {
    this.urlDialogContext_ = {
      row: DRAWN_CUSTOM_GESTURE_ROW,
      actionRows: this.drawnCustomGestureActionRows_,
      targetAction: this.drawnCustomGestureActionRows_[0],
      write: (actionRows) => this.drawnCustomGestureActionRows_ = actionRows,
    }
    this.urlDialogInput_ = this.drawnCustomGestureActionRows_[0].textParam
  }

  private onAddDrawnCustomGestureChainedActionClick_(e: Event) {
    // Rebuilding the row below replaces this button's DOM, so blur it
    // first or focus would land wherever the replacement happens to be.
    ;(e.target as HTMLElement).blur()
    this.drawnCustomGestureActionRows_ = [
      ...this.drawnCustomGestureActionRows_,
      defaultActionRow(GestureCommand.UNASSIGNED),
    ]
  }

  private onRemoveDrawnCustomGesturePrimaryActionClick_() {
    this.drawnCustomGestureActionRows_ =
      this.chainedActionRows_(this.drawnCustomGestureActionRows_)
  }

  private onDrawnCustomGestureChainedActionCommandChanged_(
    e: Event & { model: { chainedAction: ActionRow } }) {
    const command = Number((e.target as HTMLSelectElement).value)
    this.drawnCustomGestureActionRows_ = this.drawnCustomGestureActionRows_.map(
      (action) => action === e.model.chainedAction
        ? { ...action, command }
        : action)
  }

  private onDrawnCustomGestureOpenSettingsPageChanged_(e: Event) {
    const [primaryActionRow, ...chainedActionRows] = this.drawnCustomGestureActionRows_
    this.drawnCustomGestureActionRows_ = [
      {
        ...primaryActionRow,
        openSettingsPage: Number((e.target as HTMLSelectElement).value),
      },
      ...chainedActionRows,
    ]
  }

  private onDrawnCustomGestureChainedActionOpenSettingsPageChanged_(
    e: Event & { model: { chainedAction: ActionRow } }) {
    const openSettingsPage = Number((e.target as HTMLSelectElement).value)
    this.drawnCustomGestureActionRows_ = this.drawnCustomGestureActionRows_.map(
      (action) => action === e.model.chainedAction
        ? { ...action, openSettingsPage }
        : action)
  }

  private onEditDrawnCustomGestureChainedUrlClick_(
    e: Event & { model: { chainedAction: ActionRow } }) {
    this.urlDialogContext_ = {
      row: DRAWN_CUSTOM_GESTURE_ROW,
      actionRows: this.drawnCustomGestureActionRows_,
      targetAction: e.model.chainedAction,
      write: (actionRows) => this.drawnCustomGestureActionRows_ = actionRows,
    }
    this.urlDialogInput_ = e.model.chainedAction.textParam
  }

  private onRemoveDrawnCustomGestureChainedActionClick_(
    e: Event & { model: { chainedAction: ActionRow } }) {
    this.drawnCustomGestureActionRows_ = this.drawnCustomGestureActionRows_.filter(
      (action) => action !== e.model.chainedAction)
  }

  private onDrawnCustomGestureChainedActionDragStart_(
    e: DragEvent & { model: { chainedAction: ActionRow } }) {
    this.actionDragContext_ = {
      row: DRAWN_CUSTOM_GESTURE_ROW,
      actionRows: this.drawnCustomGestureActionRows_,
      draggedAction: e.model.chainedAction,
      write: (actionRows) => this.drawnCustomGestureActionRows_ = actionRows,
    }
    e.dataTransfer!.effectAllowed = 'move'
    // Some drag implementations only treat the drag as valid, and fire
    // `drop`, once `dataTransfer` actually carries data.
    e.dataTransfer!.setData('text/plain', '')
  }

  private onDrawnCustomGestureChainedActionDrop_(
    e: DragEvent & { model: { chainedAction: ActionRow } }) {
    e.preventDefault()
    const context = this.actionDragContext_
    this.actionDragContext_ = null
    if (context?.row !== DRAWN_CUSTOM_GESTURE_ROW) {
      return
    }
    context.write(moveAction(
      context.actionRows, context.draggedAction, e.model.chainedAction))
  }

  private onDrawnCustomGesturePrimaryActionDragStart_(e: DragEvent) {
    this.actionDragContext_ = {
      row: DRAWN_CUSTOM_GESTURE_ROW,
      actionRows: this.drawnCustomGestureActionRows_,
      draggedAction: this.drawnCustomGestureActionRows_[0],
      write: (actionRows) => this.drawnCustomGestureActionRows_ = actionRows,
    }
    e.dataTransfer!.effectAllowed = 'move'
    e.dataTransfer!.setData('text/plain', '')
  }

  private onDrawnCustomGesturePrimaryActionDrop_(e: DragEvent) {
    e.preventDefault()
    const context = this.actionDragContext_
    this.actionDragContext_ = null
    if (context?.row !== DRAWN_CUSTOM_GESTURE_ROW) {
      return
    }
    context.write(moveAction(
      context.actionRows, context.draggedAction,
      this.drawnCustomGestureActionRows_[0]))
  }

  private onDrawnCustomGestureMouseDown_(e: MouseEvent) {
    this.isDrawingCustomGesture_ = true
    this.drawnCustomGesturePoints_ = [this.toDrawnCustomGestureBoxPoint_(e)]
    this.hasDrawnCustomGesture_ = false
    this.updateDrawnCustomGesturePointsString_()
  }

  private onDrawnCustomGestureMouseMove_(e: MouseEvent) {
    if (!this.isDrawingCustomGesture_) {
      return
    }
    this.drawnCustomGesturePoints_.push(this.toDrawnCustomGestureBoxPoint_(e))
    this.updateDrawnCustomGesturePointsString_()
  }

  private onDrawnCustomGestureMouseUp_() {
    this.isDrawingCustomGesture_ = false
    this.hasDrawnCustomGesture_ = this.drawnCustomGesturePoints_.length >= 2
  }

  // Discards the in-progress capture immediately, bypassing the confirm
  // dialog the explicit Clear button uses.
  private onDrawnCustomGestureMouseLeave_() {
    if (!this.isDrawingCustomGesture_) {
      return
    }
    this.isDrawingCustomGesture_ = false
    this.drawnCustomGesturePoints_ = []
    this.hasDrawnCustomGesture_ = false
    this.updateDrawnCustomGesturePointsString_()
  }

  private onClearDrawnCustomGestureClick_() {
    this.showClearCustomGestureConfirmDialog_ = true
  }

  private onClearCustomGestureDialogClose_() {
    this.showClearCustomGestureConfirmDialog_ = false
  }

  private onConfirmClearCustomGestureClick_() {
    this.drawnCustomGesturePoints_ = []
    this.hasDrawnCustomGesture_ = false
    this.drawnCustomGestureActionRows_ =
      [defaultActionRow(GestureCommand.UNASSIGNED)]
    this.updateDrawnCustomGesturePointsString_()
    this.showClearCustomGestureConfirmDialog_ = false
  }

  private updateDrawnCustomGesturePointsString_() {
    this.drawnCustomGestureSvgPathData_ =
      svgPathData(this.drawnCustomGesturePoints_)
  }

  private toDrawnCustomGestureBoxPoint_(e: MouseEvent): Point {
    const box =
      this.shadowRoot!.querySelector('#drawCustomGestureBox') as HTMLElement
    const rect = box.getBoundingClientRect()
    return { x: e.clientX - rect.left, y: e.clientY - rect.top }
  }

  private onSaveDrawnCustomGestureClick_() {
    if (this.drawnCustomGesturePoints_.length < 2) {
      return
    }

    const entry: CustomGestureEntry = {
      id: `${Date.now()}`,
      points: this.drawnCustomGesturePoints_,
      actions: toActionDicts(this.drawnCustomGestureActionRows_),
    }

    this.trySaveCustomGesture_(entry)
  }

  // A conflict never blocks the save; the saved list highlights it
  // afterward instead of gating the save behind a confirmation dialog.
  private trySaveCustomGesture_(entry: CustomGestureEntry) {
    this.commitSavedCustomGesture_(entry)
  }

  private commitSavedCustomGesture_(entry: CustomGestureEntry) {
    const existingCustomGestures = this.existingCustomGestures_()
    // `appendPrefListItem` uses Polymer's push(), which only fires a
    // splice notification, not the value-changed one observers depend on.
    this.setPrefValue(
      GESTURES_CUSTOM_GESTURES_PREF, [...existingCustomGestures, entry])
    this.drawnCustomGesturePoints_ = []
    this.hasDrawnCustomGesture_ = false
    this.drawnCustomGestureActionRows_ =
      [defaultActionRow(GestureCommand.UNASSIGNED)]
    this.updateDrawnCustomGesturePointsString_()
  }

  private onRemoveCustomGestureClick_(e: DomRepeatEvent<CustomGestureActionRow>) {
    const row = e.model.item
    const customGestures = this.existingCustomGestures_()
    this.setPrefValue(
      GESTURES_CUSTOM_GESTURES_PREF,
      customGestures.filter((customGesture) => customGesture.id !== row.id))
  }
}

declare global {
  interface HTMLElementTagNameMap {
    'settings-gestures-page': SettingsGesturesPage
  }
}

customElements.define(
  SettingsGesturesPage.is, SettingsGesturesPage)
