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
  toActionDicts, defaultActionRow, buildActionRow, moveAction,
  applyCommandToActionRows, hasIntParam, IDC_OPTIONS
} from './gesture_action_util.js'
import {
  hasTextParam, primaryActionRow, chainedActionRows, hasChainedActions,
  primaryHasIntParam, primaryHasTextParam, intParamOptionsFor,
  primaryIntParamOptionsFor, isActionIncomplete, getCommandSelectClass,
  primaryCommandSelectClass, getUrlAttachClass, primaryUrlAttachClass,
  optionMatchesCommand, optionMatchesPrimaryCommand,
  optionMatchesPrimaryOpenSettingsPage, getUrlButtonLabel,
  primaryUrlButtonLabel
} from './action_row_display_util.js'
import {
  OPEN_TARGET_ORDER, OPEN_TARGET_I18N_KEYS
} from './smart_gesture_open_in_command_param.js'
import {
  SEARCH_TARGET_ORDER, SEARCH_TARGET_I18N_KEYS
} from './smart_gesture_selected_text_search_command_param.js'
import type {ActionDict} from './gesture_action_util.js'
import { withUrlScheme } from './url_scheme_util.js'
import type {
  ActionRow, DisallowedSiteRow, MouseActionRow
} from './gesture_rows.js'
import {
  findConflictingGestures, conflictDescription
} from './gesture_command_conflict.js'
import type {GestureConflicts} from './gesture_command_conflict.js'
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
  GESTURES_SMART_GESTURES_PREF, SmartGestureContentType,
  buildSmartGestureMappingActionRows
} from './smart_gesture_mapping_util.js'
import type {SmartGestureMappingActionRow} from './smart_gesture_mapping_util.js'
import {
  GESTURES_CUSTOM_GESTURES_PREF, DRAWN_CUSTOM_GESTURE_ROW,
  buildCustomGestureActionRows
} from './custom_gesture_util.js'
import type {CustomGestureActionRow, CustomGestureEntry} from './custom_gesture_util.js'
import type {Point} from './custom_gesture_geometry.js'

// Shown as the page's literal relative gesture_path (see `OPEN_SETTINGS_PAGE_PATHS`),
// not a translated label. Restricted to Brave-only pages (see
// `OpenSettingsPage`'s own comment: entries below `SHORTCUTS` mirror an
// upstream route instead), since a gesture should only jump to a page that's
// actually unique to Brave's settings, not one shared with upstream Chromium.
const OPEN_SETTINGS_PAGE_OPTIONS: Array<{ name: string, value: OpenSettingsPage }> =
  OPEN_SETTINGS_PAGE_ORDER
    .filter((value: OpenSettingsPage) =>
      value === OpenSettingsPage.BASIC ||
      value >= OpenSettingsPage.SHORTCUTS)
    .map(
      (value: OpenSettingsPage) => ({ name: OPEN_SETTINGS_PAGE_PATHS[value], value }))

// One instance per page load is fine, since the backend just adds
// another listener remote.
const commandsCache = new CommandsCache()

const GESTURES_DISALLOWED_SITES_PREF = 'brave.gestures.disallowed_sites'
const GESTURES_SENSITIVITY_IN_PIXELS_PREF =
  'brave.gestures.directional_gesture_sensitivity_in_pixels'
const GESTURES_MOUSE_ROCKER_LEFT_PREF = 'brave.gestures.mouse_rocker_left_action'
const GESTURES_MOUSE_ROCKER_RIGHT_PREF = 'brave.gestures.mouse_rocker_right_action'
const GESTURES_MOUSE_WHEEL_MODIFIER_BUTTON_PREF =
  'brave.gestures.mouse_wheel_modifier_button'
const GESTURES_MOUSE_WHEEL_UP_PREF = 'brave.gestures.mouse_wheel_up_action'
const GESTURES_MOUSE_WHEEL_DOWN_PREF = 'brave.gestures.mouse_wheel_down_action'
const GESTURES_MOUSE_WHEEL_SENSITIVITY_IN_PIXELS_PREF =
  'brave.gestures.mouse_wheel_sensitivity_in_pixels'

// Keep in sync with features::kDirectionalGestureMinStrokePixels's default
// in brave/components/gestures/core/common/features.cc.
const DEFAULT_SENSITIVITY_IN_PIXELS = 30
const SENSITIVITY_SNAP_THRESHOLD_IN_PIXELS = 2

// Keep in sync with features::kMaxDirectionalGestureStrokes's default in
// brave/components/gestures/core/common/features.cc.
const MAX_DIRECTIONAL_STROKES = 4

// Keep in sync with features::kDirectionalGestureMinStrokeStraightness's
// default in brave/components/gestures/core/common/features.cc.
const MIN_STROKE_STRAIGHTNESS = 0.9

// Keep in sync with features::kMinMouseWheelGestureDelta's default in
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
      'onSmartGesturesPrefChanged_(' +
      'prefs.brave.gestures.smart_gestures.value)',
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

      smartGestureLinkActionRows_: { type: Array, value: () => [] },
      smartGestureSelectedTextActionRows_: { type: Array, value: () => [] },
      smartGestureImageActionRows_: { type: Array, value: () => [] },
      showSmartGesturesResetConfirmDialog_: { type: Boolean, value: false },

      customGestureActionRows_: { type: Array, value: () => [] },
      drawnCustomGestureActionRows_: {
        type: Array, value: () => [defaultActionRow(GestureCommand.UNASSIGNED)]
      },
      hasDrawnCustomGesture_: { type: Boolean, value: false },
      drawnCustomGestureSvgPathData_: { type: String, value: '' },
      showClearCustomGestureConfirmDialog_: { type: Boolean, value: false },
      showRemoveCustomGestureConfirmDialog_: { type: Boolean, value: false },

      mouseRockerLeftActionRow_: {
        type: Object, value: () => (
          {
            prefName: GESTURES_MOUSE_ROCKER_LEFT_PREF,
            actionRows: [defaultActionRow(33000 /* IDC_BACK */)]
          })
      },
      mouseRockerRightActionRow_: {
        type: Object, value: () => (
          {
            prefName: GESTURES_MOUSE_ROCKER_RIGHT_PREF,
            actionRows: [defaultActionRow(33001 /* IDC_FORWARD */)]
          })
      },
      showMouseRockerResetConfirmDialog_: { type: Boolean, value: false },

      mouseWheelUpActionRow_: {
        type: Object, value: () => (
          {
            prefName: GESTURES_MOUSE_WHEEL_UP_PREF,
            actionRows: [defaultActionRow(34017 /* IDC_SELECT_PREVIOUS_TAB */)]
          })
      },
      mouseWheelDownActionRow_: {
        type: Object, value: () => (
          {
            prefName: GESTURES_MOUSE_WHEEL_DOWN_PREF,
            actionRows: [defaultActionRow(34016 /* IDC_SELECT_NEXT_TAB */)]
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
      openTargetOptions_: { type: Array, value: () => [] },
      searchTargetOptions_: { type: Array, value: () => [] },
    }
  }

  declare private directionalGestureMappingActionRows_: DirectionalGestureMappingActionRow[]
  declare private showDirectionalGestureMappingsResetConfirmDialog_: boolean

  declare private smartGestureLinkActionRows_: SmartGestureMappingActionRow[]
  declare private smartGestureSelectedTextActionRows_: SmartGestureMappingActionRow[]
  declare private smartGestureImageActionRows_: SmartGestureMappingActionRow[]
  declare private showSmartGesturesResetConfirmDialog_: boolean

  declare private customGestureActionRows_: CustomGestureActionRow[]

  private isDrawingCustomGesture_ = false
  private drawnCustomGesturePoints_: Point[] = []
  declare private drawnCustomGestureSvgPathData_: string
  declare private hasDrawnCustomGesture_: boolean
  declare private drawnCustomGestureActionRows_: ActionRow[]
  declare private showClearCustomGestureConfirmDialog_: boolean
  declare private showRemoveCustomGestureConfirmDialog_: boolean
  private pendingRemoveCustomGestureId_: string|null = null

  declare private mouseRockerLeftActionRow_: MouseActionRow
  declare private mouseRockerRightActionRow_: MouseActionRow
  declare private showMouseRockerResetConfirmDialog_: boolean

  declare private mouseWheelUpActionRow_: MouseActionRow
  declare private mouseWheelDownActionRow_: MouseActionRow
  declare private showMouseWheelResetConfirmDialog_: boolean

  declare private disallowedSiteRows_: DisallowedSiteRow[]
  declare private showAddDisallowedSiteDialog_: boolean
  declare private addDisallowedSiteInput_: string
  declare private editingDisallowedSiteOrigin_: string | null

  declare private commandOptions_: CommandOption[]
  declare private urlDialogInput_: string
  declare private openSettingsPageOptions_:
    Array<{ name: string, value: OpenSettingsPage }>
  declare private openTargetOptions_: Array<{ name: string, value: number }>
  declare private searchTargetOptions_: Array<{ name: string, value: number }>

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
    this.openTargetOptions_ = OPEN_TARGET_ORDER.map((value) => (
      { name: this.i18n(OPEN_TARGET_I18N_KEYS[value]), value }))
    this.searchTargetOptions_ = SEARCH_TARGET_ORDER.map((value) => (
      { name: this.i18n(SEARCH_TARGET_I18N_KEYS[value]), value }))
  }

  override disconnectedCallback() {
    super.disconnectedCallback()
    commandsCache.removeListener(this.boundOnCommandsChanged_)
  }

  // `UNASSIGNED` is the default, so it's pinned first unsorted; every other
  // option, sentinel or real browser command alike, is sorted alphabetically
  // by name.
  private onCommandsChanged_(commands: { [id: number]: Command }) {
    const unassignedOption = {
      name: this.i18n(SPECIAL_COMMAND_I18N_KEYS[GestureCommand.UNASSIGNED]),
      value: GestureCommand.UNASSIGNED,
    }
    const sortableSentinelOptions = [
      GestureCommand.NAVIGATE_TO_URL,
      GestureCommand.SWITCH_TO_TAB,
      GestureCommand.SEARCH_SELECTED_TEXT,
      GestureCommand.OPEN_LINK_IN,
      GestureCommand.COPY_LINK_ADDRESS,
      GestureCommand.COPY_SELECTED_TEXT,
      GestureCommand.OPEN_IMAGE_IN,
      GestureCommand.SAVE_IMAGE,
      GestureCommand.COPY_IMAGE,
    ].map((value) => (
      { name: this.i18n(SPECIAL_COMMAND_I18N_KEYS[value]), value }))
    const browserCommandOptions = Object.values(commands)
      .map((command) => ({
        // Chromium's own name for `IDC_OPTIONS` is "Options", but Brave
        // calls this concept "Settings" everywhere else in its UI.
        name: command.id === IDC_OPTIONS
          ? this.i18n('braveGesturesOpenSettingsPageCommand')
          : command.name,
        value: command.id,
      }))
    this.commandOptions_ = [
      unassignedOption,
      ...[...sortableSentinelOptions, ...browserCommandOptions]
        .sort((a, b) => a.name.localeCompare(b.name)),
    ]
  }

  private onDirectionalGestureMappingsPrefChanged_(
    overrides: Record<string, ActionDict[]> | undefined) {
    this.directionalGestureMappingActionRows_ =
      buildDirectionalGestureMappingActionRows(overrides)
  }

  private onSmartGesturesPrefChanged_(
    overrides: Record<string, ActionDict[]> | undefined) {
    this.smartGestureLinkActionRows_ =
      buildSmartGestureMappingActionRows(SmartGestureContentType.LINK, overrides)
    this.smartGestureSelectedTextActionRows_ = buildSmartGestureMappingActionRows(
      SmartGestureContentType.SELECTED_TEXT, overrides)
    this.smartGestureImageActionRows_ =
      buildSmartGestureMappingActionRows(SmartGestureContentType.IMAGE, overrides)
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

  private onMouseRockerLeftActionPrefChanged_(
    actions: ActionDict[] | undefined) {
    this.mouseRockerLeftActionRow_ = buildActionRow(
      GESTURES_MOUSE_ROCKER_LEFT_PREF, actions, 33000 /* IDC_BACK */)
  }

  private onMouseRockerRightActionPrefChanged_(
    actions: ActionDict[] | undefined) {
    this.mouseRockerRightActionRow_ = buildActionRow(
      GESTURES_MOUSE_ROCKER_RIGHT_PREF, actions, 33001 /* IDC_FORWARD */)
  }

  private onMouseWheelUpActionPrefChanged_(
    actions: ActionDict[] | undefined) {
    this.mouseWheelUpActionRow_ = buildActionRow(
      GESTURES_MOUSE_WHEEL_UP_PREF, actions, 34017 /* IDC_SELECT_PREVIOUS_TAB */)
  }

  private onMouseWheelDownActionPrefChanged_(
    actions: ActionDict[] | undefined) {
    this.mouseWheelDownActionRow_ = buildActionRow(
      GESTURES_MOUSE_WHEEL_DOWN_PREF, actions, 34016 /* IDC_SELECT_NEXT_TAB */)
  }

  // Each of the rocker's and wheel's 4 rows is its own standalone list
  // pref, so a single writer parameterized by `prefName` covers all of them.
  private writeMouseActions_(prefName: string, actionRows: ActionRow[]) {
    this.setPrefValue(prefName, toActionDicts(actionRows))
  }

  private getMouseActionRow_(prefName: string): MouseActionRow {
    switch (prefName) {
      case GESTURES_MOUSE_ROCKER_LEFT_PREF: return this.mouseRockerLeftActionRow_
      case GESTURES_MOUSE_ROCKER_RIGHT_PREF: return this.mouseRockerRightActionRow_
      case GESTURES_MOUSE_WHEEL_UP_PREF: return this.mouseWheelUpActionRow_
      case GESTURES_MOUSE_WHEEL_DOWN_PREF: return this.mouseWheelDownActionRow_
      default: throw new Error(`Unknown mouse action pref: ${prefName}`)
    }
  }

  // The rocker and wheel rows share identical dispatch, each container
  // carrying a `data-pref` attribute naming its own pref.
  private getPrefFromDataAttribute_(e: Event): string {
    return (e.currentTarget as HTMLElement).closest<HTMLElement>(
      '[data-pref]')!.dataset['pref']!
  }

  private onMouseActionCommandChanged_(e: Event) {
    const prefName = this.getPrefFromDataAttribute_(e)
    const row = this.getMouseActionRow_(prefName)
    const command = Number((e.target as HTMLSelectElement).value)
    this.writeMouseActions_(
      prefName, applyCommandToActionRows(command, row.actionRows))
  }

  private onMouseActionOpenSettingsPageChanged_(e: Event) {
    const prefName = this.getPrefFromDataAttribute_(e)
    const row = this.getMouseActionRow_(prefName)
    const openSettingsPage = Number((e.target as HTMLSelectElement).value)
    this.writeMouseActions_(prefName, [
      { ...row.actionRows[0], openSettingsPage }, ...this.chainedActionRows_(row.actionRows)])
  }

  private onMouseWheelModifierButtonChanged_(e: Event) {
    this.setPrefValue(
      GESTURES_MOUSE_WHEEL_MODIFIER_BUTTON_PREF,
      Number((e.target as HTMLSelectElement).value))
  }

  private onMouseWheelSensitivityChanged_(e: Event) {
    const input = e.target as HTMLInputElement
    const sensitivity = Number(input.value)
    const snapped = this.snapToDefault_(
      sensitivity, MOUSE_WHEEL_SENSITIVITY_SNAP_THRESHOLD_IN_PIXELS,
      DEFAULT_MOUSE_WHEEL_SENSITIVITY_IN_PIXELS)
    this.setPrefValue(GESTURES_MOUSE_WHEEL_SENSITIVITY_IN_PIXELS_PREF, snapped)
    // The pref's one-way binding won't move the slider's thumb back, since
    // a native range input only honors the `value` attribute before the
    // first user interaction.
    input.value = String(snapped)
  }

  private onEditMouseActionUrlClick_(e: Event) {
    const prefName = this.getPrefFromDataAttribute_(e)
    const row = this.getMouseActionRow_(prefName)
    this.urlDialogContext_ = {
      row, actionRows: row.actionRows, targetAction: row.actionRows[0],
      write: (actionRows) => this.writeMouseActions_(prefName, actionRows),
    }
    this.urlDialogInput_ = row.actionRows[0].textParam
  }

  private onAddMouseChainedActionClick_(e: Event) {
    // Rebuilding the row below replaces this row's DOM; blur first so focus
    // settles on the body instead of wherever it lands afterward.
    ;(e.target as HTMLElement).blur()
    const prefName = this.getPrefFromDataAttribute_(e)
    const row = this.getMouseActionRow_(prefName)
    this.writeMouseActions_(prefName, [
      ...row.actionRows,
      defaultActionRow(GestureCommand.UNASSIGNED),
    ])
  }

  // Only shown when there's at least one chained action, so there's always
  // a next action to promote into the primary slot this one vacates.
  private onRemoveMouseActionPrimaryActionClick_(e: Event) {
    const prefName = this.getPrefFromDataAttribute_(e)
    const row = this.getMouseActionRow_(prefName)
    this.writeMouseActions_(prefName, this.chainedActionRows_(row.actionRows))
  }

  private onMouseChainedActionCommandChanged_(e: Event &
    { model: { chainedAction: ActionRow } }) {
    const prefName = this.getPrefFromDataAttribute_(e)
    const row = this.getMouseActionRow_(prefName)
    const command = Number((e.target as HTMLSelectElement).value)
    this.writeMouseActions_(
      prefName, row.actionRows.map(
        (action) => action === e.model.chainedAction
          ? defaultActionRow(command)
          : action))
  }

  private onMouseChainedActionOpenSettingsPageChanged_(e: Event &
  { model: { chainedAction: ActionRow } }) {
    const prefName = this.getPrefFromDataAttribute_(e)
    const row = this.getMouseActionRow_(prefName)
    const openSettingsPage = Number((e.target as HTMLSelectElement).value)
    this.writeMouseActions_(
      prefName, row.actionRows.map(
        (action) => action === e.model.chainedAction
          ? { ...action, openSettingsPage }
          : action))
  }

  private onRemoveMouseChainedActionClick_(e: Event &
  { model: { chainedAction: ActionRow } }) {
    const prefName = this.getPrefFromDataAttribute_(e)
    const row = this.getMouseActionRow_(prefName)
    this.writeMouseActions_(
      prefName,
      row.actionRows.filter(
        (action) => action !== e.model.chainedAction))
  }

  private onEditMouseChainedUrlClick_(e: Event &
  { model: { chainedAction: ActionRow } }) {
    const prefName = this.getPrefFromDataAttribute_(e)
    const row = this.getMouseActionRow_(prefName)
    this.urlDialogContext_ = {
      row, actionRows: row.actionRows, targetAction: e.model.chainedAction,
      write: (actionRows) => this.writeMouseActions_(prefName, actionRows),
    }
    this.urlDialogInput_ = e.model.chainedAction.textParam
  }

  private onMouseActionPrimaryActionDragStart_(e: DragEvent) {
    const prefName = this.getPrefFromDataAttribute_(e)
    const row = this.getMouseActionRow_(prefName)
    this.actionDragContext_ = {
      row, actionRows: row.actionRows, draggedAction: row.actionRows[0],
      write: (actionRows) => this.writeMouseActions_(prefName, actionRows),
    }
    e.dataTransfer!.effectAllowed = 'move'
    e.dataTransfer!.setData('text/plain', '')
  }

  private onMouseActionPrimaryActionDrop_(e: DragEvent) {
    e.preventDefault()
    const prefName = this.getPrefFromDataAttribute_(e)
    const row = this.getMouseActionRow_(prefName)
    const context = this.actionDragContext_
    this.actionDragContext_ = null
    if (context?.row !== row) {
      return
    }
    context.write(moveAction(
      context.actionRows, context.draggedAction, row.actionRows[0]))
  }

  private onMouseChainedActionDragStart_(e: DragEvent &
    { model: { chainedAction: ActionRow } }) {
    const prefName = this.getPrefFromDataAttribute_(e)
    const row = this.getMouseActionRow_(prefName)
    this.actionDragContext_ = {
      row, actionRows: row.actionRows, draggedAction: e.model.chainedAction,
      write: (actionRows) => this.writeMouseActions_(prefName, actionRows),
    }
    e.dataTransfer!.effectAllowed = 'move'
    e.dataTransfer!.setData('text/plain', '')
  }

  private onMouseChainedActionDrop_(e: DragEvent &
    { model: { chainedAction: ActionRow } }) {
    e.preventDefault()
    const row = this.getMouseActionRow_(this.getPrefFromDataAttribute_(e))
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
    return hasTextParam(command)
  }

  // Whether `command` takes an `IntParamInfo`, shown via a second dropdown
  // next to the command select (see `intParamOptionsFor_` for which one).
  private hasIntParam_(command: number): boolean {
    return hasIntParam(command)
  }

  private primaryHasIntParam_(actionRows: ActionRow[]): boolean {
    return primaryHasIntParam(actionRows)
  }

  // Which options `command`'s int-param dropdown should list.
  private intParamOptionsFor_(command: number):
    Array<{ name: string, value: number }> {
    return intParamOptionsFor(
      command, this.openTargetOptions_, this.searchTargetOptions_,
      this.openSettingsPageOptions_)
  }

  private primaryIntParamOptionsFor_(actionRows: ActionRow[]):
    Array<{ name: string, value: number }> {
    return primaryIntParamOptionsFor(
      actionRows, this.openTargetOptions_, this.searchTargetOptions_,
      this.openSettingsPageOptions_)
  }

  private getCommandSelectClass_(
    command: number, textParam: string): string {
    return getCommandSelectClass(command, textParam)
  }

  // Only `textParam` has an "unset" state; every other param type defaults
  // to a value that is already valid on its own.
  private isActionIncomplete_(
    command: number, textParam: string): boolean {
    return isActionIncomplete(command, textParam)
  }

  private getUrlAttachClass_(textParam: string): string {
    return getUrlAttachClass(textParam)
  }

  // Native `<option>` reflects the `<select>`'s value via its own
  // `selected` attribute, not a one-way binding.
  private optionMatchesCommand_(
    optionValue: number, command: number): boolean {
    return optionMatchesCommand(optionValue, command)
  }

  // Template bindings cannot nest a method call inside another method
  // call's argument list, so these flatten `primaryActionRow_(actionRows)`
  // into a single call for markup that needs the primary action's fields.
  private primaryCommandSelectClass_(actionRows: ActionRow[]): string {
    return primaryCommandSelectClass(actionRows)
  }

  private optionMatchesPrimaryCommand_(
    optionValue: number, actionRows: ActionRow[]): boolean {
    return optionMatchesPrimaryCommand(optionValue, actionRows)
  }

  private optionMatchesPrimaryOpenSettingsPage_(
    optionValue: number, actionRows: ActionRow[]): boolean {
    return optionMatchesPrimaryOpenSettingsPage(optionValue, actionRows)
  }

  private primaryUrlAttachClass_(actionRows: ActionRow[]): string {
    return primaryUrlAttachClass(actionRows)
  }

  private primaryHasTextParam_(actionRows: ActionRow[]): boolean {
    return primaryHasTextParam(actionRows)
  }

  private primaryUrlButtonLabel_(actionRows: ActionRow[]): string {
    return primaryUrlButtonLabel(actionRows, this.i18n.bind(this))
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

  private getUrlButtonLabel_(
    command: number, textParam: string): string {
    return getUrlButtonLabel(command, textParam, this.i18n.bind(this))
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

  private writeSmartGestureMappingActions_(
    key: string, actionRows: ActionRow[]) {
    this.setPrefDictEntry(
      GESTURES_SMART_GESTURES_PREF, key, toActionDicts(actionRows))
  }

  private onSmartGestureMappingCommandChanged_(
    e: DomRepeatEvent<SmartGestureMappingActionRow>) {
    const row = e.model.item
    this.writeSmartGestureMappingActions_(
      row.key, applyCommandToActionRows(
        Number((e.target as HTMLSelectElement).value), row.actionRows))
  }

  private onEditSmartGestureMappingUrlClick_(
    e: DomRepeatEvent<SmartGestureMappingActionRow>) {
    const row = e.model.item
    this.urlDialogContext_ = {
      row, actionRows: row.actionRows, targetAction: row.actionRows[0],
      write: (actionRows) =>
        this.writeSmartGestureMappingActions_(row.key, actionRows),
    }
    this.urlDialogInput_ = row.actionRows[0].textParam
  }

  // Only shown when there's at least one chained action, so there's always
  // a next action to promote into the primary slot this one vacates.
  private onRemoveSmartGestureMappingPrimaryActionClick_(
    e: DomRepeatEvent<SmartGestureMappingActionRow>) {
    const row = e.model.item
    this.writeSmartGestureMappingActions_(
      row.key, this.chainedActionRows_(row.actionRows))
  }

  private onAddSmartGestureMappingChainedActionClick_(
    e: DomRepeatEvent<SmartGestureMappingActionRow>) {
    // Rebuilding the row below replaces this button's DOM, so blur it
    // first or focus would land wherever the replacement happens to be.
    ;(e.target as HTMLElement).blur()
    const row = e.model.item
    this.writeSmartGestureMappingActions_(row.key, [
      ...row.actionRows,
      defaultActionRow(GestureCommand.UNASSIGNED),
    ])
  }

  private onRemoveSmartGestureMappingChainedActionClick_(e: Event &
    { model: { chainedAction: ActionRow,
      parentModel: { item: SmartGestureMappingActionRow }
    }
    }) {
    const row = e.model.parentModel.item
    this.writeSmartGestureMappingActions_(
      row.key,
      row.actionRows.filter(
        (action) => action !== e.model.chainedAction))
  }

  private onSmartGestureMappingChainedActionCommandChanged_(e: Event &
    { model: { chainedAction: ActionRow,
      parentModel: { item: SmartGestureMappingActionRow }
    }
    }) {
    const row = e.model.parentModel.item
    const command = Number((e.target as HTMLSelectElement).value)
    this.writeSmartGestureMappingActions_(
      row.key, row.actionRows.map(
        (action) => action === e.model.chainedAction
          ? defaultActionRow(command)
          : action))
  }

  private onSmartGestureMappingOpenSettingsPageChanged_(e: Event &
  { model: { item: SmartGestureMappingActionRow } }) {
    const row = e.model.item
    const openSettingsPage = Number((e.target as HTMLSelectElement).value)
    this.writeSmartGestureMappingActions_(row.key, [
      { ...row.actionRows[0], openSettingsPage }, ...this.chainedActionRows_(row.actionRows)])
  }

  private onSmartGestureMappingChainedActionOpenSettingsPageChanged_(e: Event &
  {
    model: {
      chainedAction: ActionRow,
      parentModel: { item: SmartGestureMappingActionRow }
    }
  }) {
    const row = e.model.parentModel.item
    const openSettingsPage = Number((e.target as HTMLSelectElement).value)
    this.writeSmartGestureMappingActions_(
      row.key, row.actionRows.map(
        (action) => action === e.model.chainedAction
          ? { ...action, openSettingsPage }
          : action))
  }

  private onEditSmartGestureMappingChainedUrlClick_(e: Event &
    { model: { chainedAction: ActionRow,
      parentModel: { item: SmartGestureMappingActionRow }
    }
    }) {
    const row = e.model.parentModel.item
    this.urlDialogContext_ = {
      row, actionRows: row.actionRows, targetAction: e.model.chainedAction,
      write: (actionRows) =>
        this.writeSmartGestureMappingActions_(row.key, actionRows),
    }
    this.urlDialogInput_ = e.model.chainedAction.textParam
  }

  private onSmartGestureMappingChainedActionDragStart_(e: DragEvent &
    { model: { chainedAction: ActionRow,
      parentModel: { item: SmartGestureMappingActionRow }
    }
    }) {
    const row = e.model.parentModel.item
    this.actionDragContext_ = {
      row, actionRows: row.actionRows, draggedAction: e.model.chainedAction,
      write: (actionRows) =>
        this.writeSmartGestureMappingActions_(row.key, actionRows),
    }
    e.dataTransfer!.effectAllowed = 'move'
    // Some drag implementations only treat the drag as valid, and fire
    // `drop`, once `dataTransfer` actually carries data.
    e.dataTransfer!.setData('text/plain', '')
  }

  private onSmartGestureMappingChainedActionDrop_(e: DragEvent &
    { model: { chainedAction: ActionRow,
      parentModel: { item: SmartGestureMappingActionRow }
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

  private onSmartGestureMappingPrimaryActionDragStart_(
    e: DragEvent & { model: { item: SmartGestureMappingActionRow } }) {
    const row = e.model.item
    this.actionDragContext_ = {
      row, actionRows: row.actionRows, draggedAction: row.actionRows[0],
      write: (actionRows) =>
        this.writeSmartGestureMappingActions_(row.key, actionRows),
    }
    e.dataTransfer!.effectAllowed = 'move'
    e.dataTransfer!.setData('text/plain', '')
  }

  private onSmartGestureMappingPrimaryActionDrop_(
    e: DragEvent & { model: { item: SmartGestureMappingActionRow } }) {
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
          ? defaultActionRow(command)
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
    return primaryActionRow(actionRows)
  }

  private chainedActionRows_(actionRows: ActionRow[]): ActionRow[] {
    return chainedActionRows(actionRows)
  }

  // Flattens a method call plus property access into one call, since
  // bindings cannot chain the two.
  private hasChainedActions_(actionRows: ActionRow[]): boolean {
    return hasChainedActions(actionRows)
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
          ? defaultActionRow(command)
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

  // The 2 action-row arrays and 4 mouse rocker/wheel rows are explicit,
  // otherwise-unused parameters so this recomputes when any of them
  // changes, not just `row`.
  private directionalGestureMappingRowConflictClass_(
    row: DirectionalGestureMappingActionRow, _directionalGestureMappingRows: DirectionalGestureMappingActionRow[],
    _customGestureRows: CustomGestureActionRow[],
    _mouseRockerLeftActionRow: MouseActionRow,
    _mouseRockerRightActionRow: MouseActionRow,
    _mouseWheelUpActionRow: MouseActionRow,
    _mouseWheelDownActionRow: MouseActionRow,
    _smartGestureLinkActionRows: SmartGestureMappingActionRow[],
    _smartGestureSelectedTextActionRows: SmartGestureMappingActionRow[],
    _smartGestureImageActionRows: SmartGestureMappingActionRow[]): string {
    const conflicts = this.findConflictingGestures_(
      row.actionRows, { directionalGestureMappingKey: row.key })
    const hasConflict = this.hasAnyConflict_(conflicts) ||
      this.anyCustomGestureMatchesDirectionalGestureKey_(row.key)
    return hasConflict ? 'command-conflict' : ''
  }

  private smartGestureMappingRowConflictClass_(
    row: SmartGestureMappingActionRow, _directionalGestureMappingRows: DirectionalGestureMappingActionRow[],
    _customGestureRows: CustomGestureActionRow[],
    _mouseRockerLeftActionRow: MouseActionRow,
    _mouseRockerRightActionRow: MouseActionRow,
    _mouseWheelUpActionRow: MouseActionRow,
    _mouseWheelDownActionRow: MouseActionRow,
    _smartGestureLinkActionRows: SmartGestureMappingActionRow[],
    _smartGestureSelectedTextActionRows: SmartGestureMappingActionRow[],
    _smartGestureImageActionRows: SmartGestureMappingActionRow[]): string {
    const conflicts = this.findConflictingGestures_(
      row.actionRows, { smartGestureKey: row.key })
    return this.hasAnyConflict_(conflicts) ? 'command-conflict' : ''
  }

  private smartGestureMappingConflictDescription_(
    row: SmartGestureMappingActionRow, _directionalGestureMappingRows: DirectionalGestureMappingActionRow[],
    _customGestureRows: CustomGestureActionRow[],
    _mouseRockerLeftActionRow: MouseActionRow,
    _mouseRockerRightActionRow: MouseActionRow,
    _mouseWheelUpActionRow: MouseActionRow,
    _mouseWheelDownActionRow: MouseActionRow,
    _smartGestureLinkActionRows: SmartGestureMappingActionRow[],
    _smartGestureSelectedTextActionRows: SmartGestureMappingActionRow[],
    _smartGestureImageActionRows: SmartGestureMappingActionRow[]): string {
    const conflicts = this.findConflictingGestures_(
      row.actionRows, { smartGestureKey: row.key })
    return conflictDescription(conflicts, this.i18n.bind(this))
  }

  private anyCustomGestureMatchesDirectionalGestureKey_(key: string): boolean {
    return anyCustomGestureMatchesDirectionalGestureKey(
      key, this.existingCustomGestures_(), this.minStrokePixels_(),
      MIN_STROKE_STRAIGHTNESS, MAX_DIRECTIONAL_STROKES,
      this.directionalGestureMappingActionRows_)
  }

  private customGestureRowConflictClass_(
    row: CustomGestureActionRow, _directionalGestureMappingRows: DirectionalGestureMappingActionRow[],
    _customGestureRows: CustomGestureActionRow[],
    _mouseRockerLeftActionRow: MouseActionRow,
    _mouseRockerRightActionRow: MouseActionRow,
    _mouseWheelUpActionRow: MouseActionRow,
    _mouseWheelDownActionRow: MouseActionRow,
    _smartGestureLinkActionRows: SmartGestureMappingActionRow[],
    _smartGestureSelectedTextActionRows: SmartGestureMappingActionRow[],
    _smartGestureImageActionRows: SmartGestureMappingActionRow[]): string {
    const conflicts = this.findConflictingGestures_(
      row.actionRows, { customGestureId: row.id })
    const hasConflict = this.hasAnyConflict_(conflicts) ||
      this.customGestureShapeConflictDescription_(row.id) !== ''
    return hasConflict ? 'command-conflict' : ''
  }

  private mouseRockerRowConflictClass_(
    row: MouseActionRow, _directionalGestureMappingRows: DirectionalGestureMappingActionRow[],
    _customGestureRows: CustomGestureActionRow[],
    _mouseRockerLeftActionRow: MouseActionRow,
    _mouseRockerRightActionRow: MouseActionRow,
    _mouseWheelUpActionRow: MouseActionRow,
    _mouseWheelDownActionRow: MouseActionRow,
    _smartGestureLinkActionRows: SmartGestureMappingActionRow[],
    _smartGestureSelectedTextActionRows: SmartGestureMappingActionRow[],
    _smartGestureImageActionRows: SmartGestureMappingActionRow[]): string {
    const conflicts = this.findConflictingGestures_(
      row.actionRows, { mouseRockerPrefName: row.prefName })
    return this.hasAnyConflict_(conflicts) ? 'command-conflict' : ''
  }

  private mouseWheelRowConflictClass_(
    row: MouseActionRow, _directionalGestureMappingRows: DirectionalGestureMappingActionRow[],
    _customGestureRows: CustomGestureActionRow[],
    _mouseRockerLeftActionRow: MouseActionRow,
    _mouseRockerRightActionRow: MouseActionRow,
    _mouseWheelUpActionRow: MouseActionRow,
    _mouseWheelDownActionRow: MouseActionRow,
    _smartGestureLinkActionRows: SmartGestureMappingActionRow[],
    _smartGestureSelectedTextActionRows: SmartGestureMappingActionRow[],
    _smartGestureImageActionRows: SmartGestureMappingActionRow[]): string {
    const conflicts = this.findConflictingGestures_(
      row.actionRows, { mouseWheelPrefName: row.prefName })
    return this.hasAnyConflict_(conflicts) ? 'command-conflict' : ''
  }

  private hasAnyConflict_(conflicts: GestureConflicts): boolean {
    return conflicts.keys.length > 0 || conflicts.smartGestureKeys.length > 0 ||
      conflicts.customGestureCount > 0 ||
      conflicts.mouseRockerCount > 0 || conflicts.mouseWheelCount > 0
  }

  private findConflictingGestures_(
    actionRows: ActionRow[],
    gestureTarget: {
      directionalGestureMappingKey?: string, customGestureId?: string,
      smartGestureKey?: string,
      mouseRockerPrefName?: string, mouseWheelPrefName?: string
    }): GestureConflicts {
    return findConflictingGestures(
      actionRows, gestureTarget, this.directionalGestureMappingActionRows_,
      this.customGestureActionRows_,
      [
        ...this.smartGestureLinkActionRows_,
        ...this.smartGestureSelectedTextActionRows_,
        ...this.smartGestureImageActionRows_,
      ],
      [this.mouseRockerLeftActionRow_, this.mouseRockerRightActionRow_],
      [this.mouseWheelUpActionRow_, this.mouseWheelDownActionRow_])
  }

  private customGestureShapeConflictDescription_(id: string): string {
    return customGestureShapeConflictDescription(
      id, this.existingCustomGestures_(), this.minStrokePixels_(),
      MIN_STROKE_STRAIGHTNESS, MAX_DIRECTIONAL_STROKES,
      this.directionalGestureMappingActionRows_, this.i18n.bind(this))
  }

  private minStrokePixels_(): number {
    return this.getPref<number>(
      GESTURES_SENSITIVITY_IN_PIXELS_PREF).value ?? DEFAULT_SENSITIVITY_IN_PIXELS
  }

  private directionalGestureMappingConflictDescription_(
    row: DirectionalGestureMappingActionRow, _directionalGestureMappingRows: DirectionalGestureMappingActionRow[],
    _customGestureRows: CustomGestureActionRow[],
    _mouseRockerLeftActionRow: MouseActionRow,
    _mouseRockerRightActionRow: MouseActionRow,
    _mouseWheelUpActionRow: MouseActionRow,
    _mouseWheelDownActionRow: MouseActionRow,
    _smartGestureLinkActionRows: SmartGestureMappingActionRow[],
    _smartGestureSelectedTextActionRows: SmartGestureMappingActionRow[],
    _smartGestureImageActionRows: SmartGestureMappingActionRow[]): string {
    const conflicts = this.findConflictingGestures_(
      row.actionRows, { directionalGestureMappingKey: row.key })
    const commandConflict = conflictDescription(conflicts, this.i18n.bind(this))
    if (!this.anyCustomGestureMatchesDirectionalGestureKey_(row.key)) {
      return commandConflict
    }
    const shapeConflict = this.i18n('braveGesturesShapeConflictCustomSublabel')
    return commandConflict ? `${commandConflict}. ${shapeConflict}` : shapeConflict
  }

  private customGestureConflictDescription_(
    row: CustomGestureActionRow, _directionalGestureMappingRows: DirectionalGestureMappingActionRow[],
    _customGestureRows: CustomGestureActionRow[],
    _mouseRockerLeftActionRow: MouseActionRow,
    _mouseRockerRightActionRow: MouseActionRow,
    _mouseWheelUpActionRow: MouseActionRow,
    _mouseWheelDownActionRow: MouseActionRow,
    _smartGestureLinkActionRows: SmartGestureMappingActionRow[],
    _smartGestureSelectedTextActionRows: SmartGestureMappingActionRow[],
    _smartGestureImageActionRows: SmartGestureMappingActionRow[]): string {
    const conflicts = this.findConflictingGestures_(
      row.actionRows, { customGestureId: row.id })
    const commandConflict = conflictDescription(conflicts, this.i18n.bind(this))
    const shapeConflict = this.customGestureShapeConflictDescription_(row.id)
    if (!shapeConflict) {
      return commandConflict
    }
    return commandConflict ? `${commandConflict}. ${shapeConflict}` : shapeConflict
  }

  private mouseRockerConflictDescription_(
    row: MouseActionRow, _directionalGestureMappingRows: DirectionalGestureMappingActionRow[],
    _customGestureRows: CustomGestureActionRow[],
    _mouseRockerLeftActionRow: MouseActionRow,
    _mouseRockerRightActionRow: MouseActionRow,
    _mouseWheelUpActionRow: MouseActionRow,
    _mouseWheelDownActionRow: MouseActionRow,
    _smartGestureLinkActionRows: SmartGestureMappingActionRow[],
    _smartGestureSelectedTextActionRows: SmartGestureMappingActionRow[],
    _smartGestureImageActionRows: SmartGestureMappingActionRow[]): string {
    const conflicts = this.findConflictingGestures_(
      row.actionRows, { mouseRockerPrefName: row.prefName })
    return conflictDescription(conflicts, this.i18n.bind(this))
  }

  private mouseWheelConflictDescription_(
    row: MouseActionRow, _directionalGestureMappingRows: DirectionalGestureMappingActionRow[],
    _customGestureRows: CustomGestureActionRow[],
    _mouseRockerLeftActionRow: MouseActionRow,
    _mouseRockerRightActionRow: MouseActionRow,
    _mouseWheelUpActionRow: MouseActionRow,
    _mouseWheelDownActionRow: MouseActionRow,
    _smartGestureLinkActionRows: SmartGestureMappingActionRow[],
    _smartGestureSelectedTextActionRows: SmartGestureMappingActionRow[],
    _smartGestureImageActionRows: SmartGestureMappingActionRow[]): string {
    const conflicts = this.findConflictingGestures_(
      row.actionRows, { mouseWheelPrefName: row.prefName })
    return conflictDescription(conflicts, this.i18n.bind(this))
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

  private onResetSmartGesturesClick_() {
    this.showSmartGesturesResetConfirmDialog_ = true
  }

  private onSmartGesturesResetDialogClose_() {
    this.showSmartGesturesResetConfirmDialog_ = false
  }

  private onConfirmSmartGesturesResetClick_() {
    for (const row of [
      ...this.smartGestureLinkActionRows_,
      ...this.smartGestureSelectedTextActionRows_,
      ...this.smartGestureImageActionRows_,
    ]) {
      this.deletePrefDictEntry(GESTURES_SMART_GESTURES_PREF, row.key)
    }
    this.showSmartGesturesResetConfirmDialog_ = false
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
    this.writeMouseActions_(
      GESTURES_MOUSE_ROCKER_LEFT_PREF, [defaultActionRow(33000 /* IDC_BACK */)])
    this.writeMouseActions_(
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
    this.writeMouseActions_(
      GESTURES_MOUSE_WHEEL_UP_PREF,
      [defaultActionRow(34017 /* IDC_SELECT_PREVIOUS_TAB */)])
    this.writeMouseActions_(
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
    // A bare host or http(s) URL normalizes down to just its hostname, but
    // an explicit non-http(s) scheme (e.g. brave://settings) is kept, since
    // it means one specific page rather than "this site."
    const input = this.addDisallowedSiteInput_.trim()
    if (!input) {
      return
    }
    let key = input
    try {
      const url = new URL(input.includes('://') ? input : `https://${input}`)
      const scheme = url.protocol.replace(':', '')
      key = scheme === 'http' || scheme === 'https'
        ? url.hostname
        : `${scheme}://${url.hostname}`
    } catch {
      // Not a parseable URL; fall back to using the trimmed input as-is.
    }
    // Guarantees every stored key is lowercase, so matching it never needs
    // to, even though `URL` already lowercases the parseable case above.
    key = key.toLowerCase()
    if (this.editingDisallowedSiteOrigin_ !== null &&
        this.editingDisallowedSiteOrigin_ !== key) {
      this.deletePrefDictEntry(
        GESTURES_DISALLOWED_SITES_PREF, this.editingDisallowedSiteOrigin_)
    }
    this.setPrefDictEntry(GESTURES_DISALLOWED_SITES_PREF, key, true)
    this.showAddDisallowedSiteDialog_ = false
  }

  private onRemoveDisallowedSiteClick_(e: DomRepeatEvent<DisallowedSiteRow>) {
    const row = e.model.item
    this.deletePrefDictEntry(GESTURES_DISALLOWED_SITES_PREF, row.origin)
  }

  private onSensitivityChanged_(e: Event) {
    const input = e.target as HTMLInputElement
    const sensitivity = Number(input.value)
    const snapped = this.snapToDefault_(
      sensitivity, SENSITIVITY_SNAP_THRESHOLD_IN_PIXELS, DEFAULT_SENSITIVITY_IN_PIXELS)
    this.setPrefValue(GESTURES_SENSITIVITY_IN_PIXELS_PREF, snapped)
    // The pref's one-way binding won't move the slider's thumb back, since
    // a native range input only honors the `value` attribute before the
    // first user interaction.
    input.value = String(snapped)
  }

  // Lands exactly on the default when released within `threshold` of it, so
  // it is easy to return to the out-of-the-box value.
  private snapToDefault_(
    value: number, threshold: number, defaultValue: number): number {
    return Math.abs(value - defaultValue) <= threshold ? defaultValue : value
  }

  private onDrawnCustomGestureCommandChanged_(e: Event) {
    const command = Number((e.target as HTMLSelectElement).value)
    this.drawnCustomGestureActionRows_ =
      applyCommandToActionRows(command, this.drawnCustomGestureActionRows_)
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
        ? defaultActionRow(command)
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

    this.commitSavedCustomGesture_(entry)
  }

  // A conflict never blocks the save; the saved list highlights it
  // afterward instead of gating the save behind a confirmation dialog.
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
    this.pendingRemoveCustomGestureId_ = e.model.item.id
    this.showRemoveCustomGestureConfirmDialog_ = true
  }

  private onRemoveCustomGestureDialogClose_() {
    this.showRemoveCustomGestureConfirmDialog_ = false
    this.pendingRemoveCustomGestureId_ = null
  }

  private onConfirmRemoveCustomGestureClick_() {
    const id = this.pendingRemoveCustomGestureId_
    const customGestures = this.existingCustomGestures_()
    this.setPrefValue(
      GESTURES_CUSTOM_GESTURES_PREF,
      customGestures.filter((customGesture) => customGesture.id !== id))
    this.showRemoveCustomGestureConfirmDialog_ = false
    this.pendingRemoveCustomGestureId_ = null
  }
}

declare global {
  interface HTMLElementTagNameMap {
    'settings-gestures-page': SettingsGesturesPage
  }
}

customElements.define(
  SettingsGesturesPage.is, SettingsGesturesPage)
