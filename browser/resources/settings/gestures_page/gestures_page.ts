/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import '../settings_shared.css.js'
import 'chrome://resources/brave/leo.bundle.js'
import './gesture_action_chain_group.js'
import type {
  EditUrlClickEventDetail, OptionSets
} from './gesture_action_chain_group.js'

import {PrefsMixin} from '/shared/settings/prefs/prefs_mixin.js'
import {I18nMixin} from 'chrome://resources/cr_elements/i18n_mixin.js'
import {loadTimeData} from '../i18n_setup.js'
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
  toActionDicts, defaultActionRow, buildActionRow, IDC_OPTIONS,
  getUrlDialogValue
} from './gesture_action_util.js'
import {optionMatchesCommand} from './action_row_display_util.js'
import {
  OPEN_TARGET_ORDER, OPEN_TARGET_I18N_KEYS
} from './smart_gesture_open_in_command_param.js'
import {
  SEARCH_TARGET_ORDER, SEARCH_TARGET_I18N_KEYS
} from './smart_gesture_selected_text_search_command_param.js'
import type {ActionDict} from './gesture_action_util.js'
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
import {shouldAcceptDrawnPoint} from './custom_gesture_geometry.js'
import {
  addDisallowedSiteDialogTitle, disallowedSiteRowsFromPref,
  toDisallowedSiteOrigin
} from './disallowed_site_util.js'

// Keeps BASIC plus the Brave-specific pages (value >= SHORTCUTS, no
// upstream route), labeled with the raw `OPEN_SETTINGS_PAGE_PATHS` path.
const OPEN_SETTINGS_PAGE_OPTIONS: Array<{ name: string, value: OpenSettingsPage }> =
  OPEN_SETTINGS_PAGE_ORDER
    .filter((value: OpenSettingsPage) =>
      value === OpenSettingsPage.BASIC ||
      value >= OpenSettingsPage.SHORTCUTS)
    .map(
      (value: OpenSettingsPage) => ({ name: OPEN_SETTINGS_PAGE_PATHS[value], value }))

// One instance per page load is fine, since the backend just adds another
// listener remote.
const commandsCache = new CommandsCache()

const GESTURES_DISALLOWED_SITES_PREF = 'brave.gestures.disallowed_sites'
const GESTURES_DIRECTIONAL_GESTURE_SENSITIVITY_IN_PIXELS_PREF =
  'brave.gestures.directional_gesture_sensitivity_in_pixels'
const GESTURES_MOUSE_ROCKER_LEFT_PREF = 'brave.gestures.mouse_rocker_left_action'
const GESTURES_MOUSE_ROCKER_RIGHT_PREF = 'brave.gestures.mouse_rocker_right_action'
const GESTURES_MOUSE_WHEEL_MODIFIER_BUTTON_PREF =
  'brave.gestures.mouse_wheel_modifier_button'
const GESTURES_MOUSE_WHEEL_UP_PREF = 'brave.gestures.mouse_wheel_up_action'
const GESTURES_MOUSE_WHEEL_DOWN_PREF = 'brave.gestures.mouse_wheel_down_action'
const GESTURES_MOUSE_WHEEL_SENSITIVITY_IN_PIXELS_PREF =
  'brave.gestures.mouse_wheel_sensitivity_in_pixels'

const DEFAULT_DIRECTIONAL_GESTURE_SENSITIVITY_IN_PIXELS =
  loadTimeData.getValue<number>('directionalGestureSensitivityInPixels')
const DIRECTIONAL_GESTURE_SENSITIVITY_SNAP_THRESHOLD_IN_PIXELS = 2

const MAX_DIRECTIONAL_GESTURE_STROKES =
  loadTimeData.getInteger('maxDirectionalGestureStrokes')

const DIRECTIONAL_GESTURE_MIN_STROKE_STRAIGHTNESS =
  loadTimeData.getValue<number>('directionalGestureMinStrokeStraightness')

const DEFAULT_MOUSE_WHEEL_SENSITIVITY_IN_PIXELS =
  loadTimeData.getValue<number>('mouseWheelSensitivityInPixels')
const MOUSE_WHEEL_SENSITIVITY_SNAP_THRESHOLD_IN_PIXELS = 2

const DEFAULT_MOUSE_WHEEL_MODIFIER_BUTTON =
  loadTimeData.getInteger('mouseWheelModifierButtonDefault')

const CUSTOM_GESTURE_DRAW_MIN_POINT_DISTANCE_PIXELS =
  loadTimeData.getInteger('customGestureDrawMinPointDistancePixels')
const CUSTOM_GESTURE_DRAW_MAX_POINT_COUNT =
  loadTimeData.getInteger('customGestureDrawMaxPointCount')

const SettingsGesturesPageBase =
  SettingsViewMixin(PrefsMixin(I18nMixin(PolymerElement)))

class SettingsGesturesPage extends SettingsGesturesPageBase {
  static get is() {
    return 'settings-gestures-page'
  }

  static get template() {
    return getTemplate()
  }

  // Not read in `ready`: `prefs` populates asynchronously, so a sync read
  // there would hit `PrefsMixin.getPref`'s "Pref is missing" assert first.
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
      'updateConflictsSignature_(' +
      'directionalGestureMappingActionRows_, customGestureActionRows_, ' +
      'mouseRockerLeftActionRow_, mouseRockerRightActionRow_, ' +
      'mouseWheelUpActionRow_, mouseWheelDownActionRow_, ' +
      'smartGestureLinkActionRows_, smartGestureSelectedTextActionRows_, ' +
      'smartGestureImageActionRows_)',
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

      // Bundles the 4 option lists above, since every
      // gesture-action-chain-group binds all 4 and none varies per row.
      optionSets_: {
        type: Object,
        computed: 'computeOptionSets_(' +
          'commandOptions_, openTargetOptions_, searchTargetOptions_, ' +
          'openSettingsPageOptions_)',
      },

      // Bumped by `updateConflictsSignature_` whenever any row that can
      // conflict with another changes, so conflict-check bindings can
      // depend on this one property instead of listing every row.
      conflictsSignature_: { type: Number, value: 0 },
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
  declare private optionSets_: OptionSets
  declare private conflictsSignature_: number

  // One dialog shared across row types; `row` identifies which row may
  // confirm it, `write` persists the result back to that row.
  declare private urlDialogContext_: {
    row: unknown
    actionRows: ActionRow[]
    targetAction: ActionRow
    write: (actionRows: ActionRow[]) => void
  } | null

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

  private computeOptionSets_(
    commandOptions: CommandOption[],
    openTargetOptions: Array<{ name: string, value: number }>,
    searchTargetOptions: Array<{ name: string, value: number }>,
    openSettingsPageOptions: Array<{ name: string, value: OpenSettingsPage }>):
    OptionSets {
    return {
      commandOptions, openTargetOptions, searchTargetOptions,
      openSettingsPageOptions,
    }
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
    this.disallowedSiteRows_ = disallowedSiteRowsFromPref(disallowedSites)
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

  private onMouseWheelModifierButtonChanged_(e: Event) {
    this.setPrefValue(
      GESTURES_MOUSE_WHEEL_MODIFIER_BUTTON_PREF,
      Number((e.target as HTMLSelectElement).value))
  }

  // Native `<option>` reflects the `<select>`'s value via its own
  // `selected` attribute, not a one-way binding.
  private optionMatchesCommand_(
    optionValue: number, command: number): boolean {
    return optionMatchesCommand(optionValue, command)
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

  private onMouseActionRowsChanged_(e: CustomEvent<{ value: ActionRow[] }>) {
    this.writeMouseActions_(this.getPrefFromDataAttribute_(e), e.detail.value)
  }

  private onMouseEditUrlClick_(e: CustomEvent<EditUrlClickEventDetail>) {
    const prefName = this.getPrefFromDataAttribute_(e)
    this.urlDialogContext_ = {
      row: this.getMouseActionRow_(prefName),
      actionRows: e.detail.actionRows, targetAction: e.detail.targetAction,
      write: (actionRows) => this.writeMouseActions_(prefName, actionRows),
    }
    this.urlDialogInput_ = e.detail.targetAction.textParam
  }

  private onDirectionalGestureMappingActionRowsChanged_(
    e: CustomEvent<{ value: ActionRow[] }> &
    { model: { item: DirectionalGestureMappingActionRow } }) {
    this.writeDirectionalGestureMappingActions_(
      e.model.item.key, e.detail.value)
  }

  private onDirectionalGestureMappingEditUrlClick_(
    e: CustomEvent<EditUrlClickEventDetail> &
    { model: { item: DirectionalGestureMappingActionRow } }) {
    const row = e.model.item
    this.urlDialogContext_ = {
      row, actionRows: e.detail.actionRows,
      targetAction: e.detail.targetAction,
      write: (actionRows) =>
        this.writeDirectionalGestureMappingActions_(row.key, actionRows),
    }
    this.urlDialogInput_ = e.detail.targetAction.textParam
  }

  private onCustomGestureActionRowsChanged_(
    e: CustomEvent<{ value: ActionRow[] }> &
    { model: { item: CustomGestureActionRow } }) {
    this.writeCustomGestureActions_(e.model.item.id, e.detail.value)
  }

  private onCustomGestureEditUrlClick_(
    e: CustomEvent<EditUrlClickEventDetail> &
    { model: { item: CustomGestureActionRow } }) {
    const row = e.model.item
    this.urlDialogContext_ = {
      row, actionRows: e.detail.actionRows,
      targetAction: e.detail.targetAction,
      write: (actionRows) => this.writeCustomGestureActions_(row.id, actionRows),
    }
    this.urlDialogInput_ = e.detail.targetAction.textParam
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
    const url = getUrlDialogValue(
      context.targetAction.command, this.urlDialogInput_)
    context.write(context.actionRows.map((action) =>
      action === context.targetAction ? { ...action, textParam: url } : action))
    this.urlDialogContext_ = null
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

  private onSmartGestureMappingActionRowsChanged_(
    e: CustomEvent<{ value: ActionRow[] }> &
    { model: { item: SmartGestureMappingActionRow } }) {
    this.writeSmartGestureMappingActions_(e.model.item.key, e.detail.value)
  }

  private onSmartGestureMappingEditUrlClick_(
    e: CustomEvent<EditUrlClickEventDetail> &
    { model: { item: SmartGestureMappingActionRow } }) {
    const row = e.model.item
    this.urlDialogContext_ = {
      row, actionRows: e.detail.actionRows,
      targetAction: e.detail.targetAction,
      write: (actionRows) =>
        this.writeSmartGestureMappingActions_(row.key, actionRows),
    }
    this.urlDialogInput_ = e.detail.targetAction.textParam
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

  // Bumped whenever any row that can conflict with another changes; see
  // `conflictsSignature_`'s own comment.
  private updateConflictsSignature_() {
    this.conflictsSignature_ = this.conflictsSignature_ + 1
  }

  // `_conflictsSignature` is an otherwise-unused parameter so this
  // recomputes whenever any row changes, not just `row`.
  private directionalGestureMappingHasConflict_(
    row: DirectionalGestureMappingActionRow,
    _conflictsSignature: number): boolean {
    const conflicts = this.findConflictingGestures_(
      row.actionRows, { directionalGestureMappingKey: row.key })
    return this.hasAnyConflict_(conflicts) ||
      this.anyCustomGestureMatchesDirectionalGestureKey_(row.key)
  }

  private smartGestureMappingHasConflict_(
    row: SmartGestureMappingActionRow, _conflictsSignature: number): boolean {
    const conflicts = this.findConflictingGestures_(
      row.actionRows, { smartGestureKey: row.key })
    return this.hasAnyConflict_(conflicts)
  }

  private smartGestureMappingConflictDescription_(
    row: SmartGestureMappingActionRow, _conflictsSignature: number): string {
    const conflicts = this.findConflictingGestures_(
      row.actionRows, { smartGestureKey: row.key })
    return conflictDescription(conflicts, this.i18n.bind(this))
  }

  private anyCustomGestureMatchesDirectionalGestureKey_(key: string): boolean {
    return anyCustomGestureMatchesDirectionalGestureKey(
      key, this.existingCustomGestures_(), this.minStrokePixels_(),
      DIRECTIONAL_GESTURE_MIN_STROKE_STRAIGHTNESS, MAX_DIRECTIONAL_GESTURE_STROKES,
      this.directionalGestureMappingActionRows_)
  }

  private customGestureHasConflict_(
    row: CustomGestureActionRow, _conflictsSignature: number): boolean {
    const conflicts = this.findConflictingGestures_(
      row.actionRows, { customGestureId: row.id })
    return this.hasAnyConflict_(conflicts) ||
      this.customGestureShapeConflictDescription_(row.id) !== ''
  }

  private mouseRockerHasConflict_(
    row: MouseActionRow, _conflictsSignature: number): boolean {
    const conflicts = this.findConflictingGestures_(
      row.actionRows, { mouseRockerPrefName: row.prefName })
    return this.hasAnyConflict_(conflicts)
  }

  private mouseWheelHasConflict_(
    row: MouseActionRow, _conflictsSignature: number): boolean {
    const conflicts = this.findConflictingGestures_(
      row.actionRows, { mouseWheelPrefName: row.prefName })
    return this.hasAnyConflict_(conflicts)
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
      [this.mouseWheelUpActionRow_, this.mouseWheelDownActionRow_],
      this.i18n.bind(this))
  }

  private customGestureShapeConflictDescription_(id: string): string {
    return customGestureShapeConflictDescription(
      id, this.existingCustomGestures_(), this.minStrokePixels_(),
      DIRECTIONAL_GESTURE_MIN_STROKE_STRAIGHTNESS, MAX_DIRECTIONAL_GESTURE_STROKES,
      this.directionalGestureMappingActionRows_, this.i18n.bind(this))
  }

  private minStrokePixels_(): number {
    return this.getPref<number>(
      GESTURES_DIRECTIONAL_GESTURE_SENSITIVITY_IN_PIXELS_PREF).value ?? DEFAULT_DIRECTIONAL_GESTURE_SENSITIVITY_IN_PIXELS
  }

  private directionalGestureMappingConflictDescription_(
    row: DirectionalGestureMappingActionRow, _conflictsSignature: number): string {
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
    row: CustomGestureActionRow, _conflictsSignature: number): string {
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
    row: MouseActionRow, _conflictsSignature: number): string {
    const conflicts = this.findConflictingGestures_(
      row.actionRows, { mouseRockerPrefName: row.prefName })
    return conflictDescription(conflicts, this.i18n.bind(this))
  }

  private mouseWheelConflictDescription_(
    row: MouseActionRow, _conflictsSignature: number): string {
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

  private getAddDisallowedSiteDialogTitle_(
    editingDisallowedSiteOrigin: string | null): string {
    return addDisallowedSiteDialogTitle(
      editingDisallowedSiteOrigin, this.i18n.bind(this))
  }

  private onAddSiteDialogClose_() {
    this.showAddDisallowedSiteDialog_ = false
  }

  private onAddSiteInputChanged_(e: Event) {
    this.addDisallowedSiteInput_ = (e.target as HTMLInputElement).value
  }

  private onConfirmAddSiteClick_() {
    const disallowedSiteOrigin =
      toDisallowedSiteOrigin(this.addDisallowedSiteInput_)
    if (!disallowedSiteOrigin) {
      return
    }
    if (this.editingDisallowedSiteOrigin_ !== null &&
      this.editingDisallowedSiteOrigin_ !== disallowedSiteOrigin) {
      this.deletePrefDictEntry(
        GESTURES_DISALLOWED_SITES_PREF, this.editingDisallowedSiteOrigin_)
    }
    this.setPrefDictEntry(
      GESTURES_DISALLOWED_SITES_PREF, disallowedSiteOrigin, true)
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
      sensitivity, DIRECTIONAL_GESTURE_SENSITIVITY_SNAP_THRESHOLD_IN_PIXELS, DEFAULT_DIRECTIONAL_GESTURE_SENSITIVITY_IN_PIXELS)
    this.setPrefValue(GESTURES_DIRECTIONAL_GESTURE_SENSITIVITY_IN_PIXELS_PREF, snapped)
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

  private onDrawnCustomGestureActionRowsChanged_(
    e: CustomEvent<{ value: ActionRow[] }>) {
    this.drawnCustomGestureActionRows_ = e.detail.value
  }

  private onDrawnCustomGestureEditUrlClick_(e: CustomEvent<EditUrlClickEventDetail>) {
    this.urlDialogContext_ = {
      row: DRAWN_CUSTOM_GESTURE_ROW,
      actionRows: e.detail.actionRows,
      targetAction: e.detail.targetAction,
      write: (actionRows) => this.drawnCustomGestureActionRows_ = actionRows,
    }
    this.urlDialogInput_ = e.detail.targetAction.textParam
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
    const point = this.toDrawnCustomGestureBoxPoint_(e)
    if (!shouldAcceptDrawnPoint(
        this.drawnCustomGesturePoints_, point,
        CUSTOM_GESTURE_DRAW_MIN_POINT_DISTANCE_PIXELS,
        CUSTOM_GESTURE_DRAW_MAX_POINT_COUNT)) {
      return
    }
    this.drawnCustomGesturePoints_.push(point)
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
