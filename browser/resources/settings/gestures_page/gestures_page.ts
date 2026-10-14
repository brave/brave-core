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
import {GestureCommand, COMMAND_ORDER, COMMAND_I18N_KEYS,
  DEFAULT_GESTURE_MAPPINGS, gestureKeyToIconNames, withUrlScheme}
  from './gesture_command.js'
import {closestCustomGestureDistance, smoothedSvgPath, fitToViewBox,
  CUSTOM_GESTURE_SIMILARITY_WARNING_THRESHOLD}
  from './custom_gesture_geometry.js'
import {classifyDirectionalStrokeKey} from './directional_gesture_classifier.js'
import {buildActionEntries, textParamFromEntry} from './gesture_rows.js'
import type {ChainedActionRow, ChainedActionEntry, MappingRow,
  CommandOption, DisallowedSiteRow, CustomGestureRow, CustomGesturePoint,
  CustomGestureEntry, RockerWheelRow} from './gesture_rows.js'

const GESTURES_MAPPINGS_PREF = 'brave.gestures.directional_gestures'
const GESTURES_DISALLOWED_SITES_PREF = 'brave.gestures.disallowed_sites'
const GESTURES_SENSITIVITY_IN_PIXELS_PREF =
  'brave.gestures.sensitivity_in_pixels'
const GESTURES_CUSTOM_GESTURES_PREF = 'brave.gestures.custom_gestures'
const GESTURES_ROCK_LEFT_PREF = 'brave.gestures.mouse_rocker_left_action'
const GESTURES_ROCK_RIGHT_PREF = 'brave.gestures.mouse_rocker_right_action'
const GESTURES_WHEEL_MODIFIER_BUTTON_PREF =
  'brave.gestures.mouse_wheel_modifier_button'
const GESTURES_WHEEL_UP_PREF = 'brave.gestures.mouse_wheel_up_action'
const GESTURES_WHEEL_DOWN_PREF = 'brave.gestures.mouse_wheel_down_action'
const GESTURES_WHEEL_SENSITIVITY_IN_PIXELS_PREF =
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
const DEFAULT_WHEEL_SENSITIVITY_IN_PIXELS = 0
const WHEEL_SENSITIVITY_SNAP_THRESHOLD_IN_PIXELS = 2

// Keep in sync with `kMouseWheelModifierButton`'s registered default in
// brave/components/gestures/core/browser/prefs_registration.cc.
const DEFAULT_WHEEL_MODIFIER_BUTTON = 2 // MouseWheelModifierButtonType::kRight

const CUSTOM_GESTURE_PREVIEW_SIZE = 32
const CUSTOM_GESTURE_PREVIEW_PADDING = 4

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
      'onMappingsPrefChanged_(' +
      'prefs.brave.gestures.directional_gestures.value)',
      'onDisallowedSitesPrefChanged_(' +
      'prefs.brave.gestures.disallowed_sites.value)',
      'onCustomGesturesPrefChanged_(' +
      'prefs.brave.gestures.custom_gestures.value)',
      'onRockLeftActionPrefChanged_(' +
      'prefs.brave.gestures.mouse_rocker_left_action.value)',
      'onRockRightActionPrefChanged_(' +
      'prefs.brave.gestures.mouse_rocker_right_action.value)',
      'onWheelUpActionPrefChanged_(' +
      'prefs.brave.gestures.mouse_wheel_up_action.value)',
      'onWheelDownActionPrefChanged_(' +
      'prefs.brave.gestures.mouse_wheel_down_action.value)',
    ]
  }

  static get properties() {
    return {
      mappingRows_: { type: Array, value: () => [] },
      disallowedSiteRows_: { type: Array, value: () => [] },
      customGestureRows_: { type: Array, value: () => [] },
      selectedCaptureCommand_:
        { type: Number, value: GestureCommand.UNASSIGNED },
      selectedCaptureNavigateToUrl_: { type: String, value: '' },
      capturedChainedActions_: { type: Array, value: () => [] },
      hasCapturedGesture_: { type: Boolean, value: false },
      commandOptions_: { type: Array, value: () => [] },
      capturedPathData_: { type: String, value: '' },
      showResetConfirmDialog_: { type: Boolean, value: false },
      showRockerResetConfirmDialog_: { type: Boolean, value: false },
      showWheelResetConfirmDialog_: { type: Boolean, value: false },
      showAddSiteDialog_: { type: Boolean, value: false },
      addSiteInput_: { type: String, value: '' },
      isEditingSite_: { type: Boolean, value: false },
      showClearGestureConfirmDialog_: { type: Boolean, value: false },
      showUrlDialog_: { type: Boolean, value: false },
      urlDialogInput_: { type: String, value: '' },
      rockLeftRow_: { type: Object, value: () => (
        { prefName: GESTURES_ROCK_LEFT_PREF, gestureCommand: GestureCommand.BACK,
          navigateToUrl: '', chainedActions: [] }) },
      rockRightRow_: { type: Object, value: () => (
        { prefName: GESTURES_ROCK_RIGHT_PREF,
          gestureCommand: GestureCommand.FORWARD, navigateToUrl: '',
          chainedActions: [] }) },
      wheelUpRow_: { type: Object, value: () => (
        { prefName: GESTURES_WHEEL_UP_PREF,
          gestureCommand: GestureCommand.PREVIOUS_TAB, navigateToUrl: '',
          chainedActions: [] }) },
      wheelDownRow_: { type: Object, value: () => (
        { prefName: GESTURES_WHEEL_DOWN_PREF,
          gestureCommand: GestureCommand.NEXT_TAB, navigateToUrl: '',
          chainedActions: [] }) },
    }
  }

  declare private mappingRows_: MappingRow[]
  declare private disallowedSiteRows_: DisallowedSiteRow[]
  declare private customGestureRows_: CustomGestureRow[]
  declare private selectedCaptureCommand_: GestureCommand
  declare private selectedCaptureNavigateToUrl_: string
  declare private capturedChainedActions_: ChainedActionRow[]
  declare private hasCapturedGesture_: boolean
  declare private commandOptions_: CommandOption[]
  declare private capturedPathData_: string
  declare private showResetConfirmDialog_: boolean
  declare private showRockerResetConfirmDialog_: boolean
  declare private showWheelResetConfirmDialog_: boolean
  declare private showAddSiteDialog_: boolean
  declare private addSiteInput_: string
  declare private isEditingSite_: boolean
  declare private showClearGestureConfirmDialog_: boolean
  declare private showUrlDialog_: boolean
  declare private urlDialogInput_: string
  declare private rockLeftRow_: RockerWheelRow
  declare private rockRightRow_: RockerWheelRow
  declare private wheelUpRow_: RockerWheelRow
  declare private wheelDownRow_: RockerWheelRow

  private capturedPoints_: CustomGesturePoint[] = []
  private isCapturing_ = false

  // The origin being edited, or null when the dialog is adding a new one.
  private editingSiteOrigin_: string|null = null

  // Which row/chained-action the URL/pattern dialog is currently editing,
  // set when it opens and consulted only on confirm; chainedAction is
  // undefined when editing a row's own primary action rather than one of
  // its chained ones. `gestureCommand` is the action's own command at the
  // time the dialog opened, since the same dialog is shared between
  // Navigate to URL and Switch to Tab (see `hasTextParam_`), which need
  // different title/placeholder text and whether the input is treated as
  // a URL (schemed) or a glob pattern (left as-is).
  private urlDialogContext_: {
    mappingRow?: MappingRow
    customGestureRow?: CustomGestureRow
    isCapture?: boolean
    chainedAction?: ChainedActionRow
    rockerWheelRow?: RockerWheelRow
    gestureCommand: GestureCommand
  } | null = null

  // Set on dragstart, consulted and cleared on drop (or dragend, if the
  // drag was abandoned without a drop). `draggedAction` is null when the
  // primary action itself (rather than one of its chained actions) is
  // being dragged.
  private dragContext_: {
    mappingRow?: MappingRow
    customGestureRow?: CustomGestureRow
    isCapture?: boolean
    rockerWheelRow?: RockerWheelRow
    draggedAction: ChainedActionRow | null
  } | null = null

  override ready() {
    super.ready()
    this.commandOptions_ = COMMAND_ORDER.map(
      (value) => ({ name: this.i18n(COMMAND_I18N_KEYS[value]), value }))
  }

  private onMappingsPrefChanged_(
    overrides: Record<string, ChainedActionEntry[]> | undefined) {
    this.mappingRows_ = DEFAULT_GESTURE_MAPPINGS.map(
      ([key, defaultCommand]) => {
        const actions = (overrides ?? {})[key]
        return {
          key,
          icons: gestureKeyToIconNames(key),
          gestureCommand: actions?.[0]?.command ?? defaultCommand,
          navigateToUrl: actions?.[0] ? textParamFromEntry(actions[0]) : '',
          chainedActions: (actions ?? []).slice(1).map((entry) => ({
            gestureCommand: entry.command,
            navigateToUrl: textParamFromEntry(entry),
          })),
        }
      })
  }

  private onDisallowedSitesPrefChanged_(
    disallowedSites: Record<string, boolean> | undefined) {
    this.disallowedSiteRows_ =
      Object.keys(disallowedSites ?? {}).map((origin) => ({ origin }))
  }

  private onCustomGesturesPrefChanged_(
    customGestures: CustomGestureEntry[] | undefined) {
    this.customGestureRows_ = (customGestures ?? []).map((customGesture) => ({
      id: customGesture.id,
      gestureCommand:
        customGesture.actions?.[0]?.command ?? GestureCommand.UNASSIGNED,
      previewPath: customGesture.points.length >= 2
        ? smoothedSvgPath(
          fitToViewBox(customGesture.points, CUSTOM_GESTURE_PREVIEW_SIZE,
            CUSTOM_GESTURE_PREVIEW_PADDING))
        : '',
      navigateToUrl: customGesture.actions?.[0]
        ? textParamFromEntry(customGesture.actions[0]) : '',
      chainedActions: (customGesture.actions ?? []).slice(1).map((entry) => ({
        gestureCommand: entry.command,
        navigateToUrl: textParamFromEntry(entry),
      })),
    }))
  }

  private buildRockerWheelRow_(
      prefName: string, actions: ChainedActionEntry[] | undefined,
      defaultCommand: GestureCommand): RockerWheelRow {
    return {
      prefName,
      gestureCommand: actions?.[0]?.command ?? defaultCommand,
      navigateToUrl: actions?.[0] ? textParamFromEntry(actions[0]) : '',
      chainedActions: (actions ?? []).slice(1).map((entry) => ({
        gestureCommand: entry.command,
        navigateToUrl: textParamFromEntry(entry),
      })),
    }
  }

  private onRockLeftActionPrefChanged_(
    actions: ChainedActionEntry[] | undefined) {
    this.rockLeftRow_ = this.buildRockerWheelRow_(
      GESTURES_ROCK_LEFT_PREF, actions, GestureCommand.BACK)
  }

  private onRockRightActionPrefChanged_(
    actions: ChainedActionEntry[] | undefined) {
    this.rockRightRow_ = this.buildRockerWheelRow_(
      GESTURES_ROCK_RIGHT_PREF, actions, GestureCommand.FORWARD)
  }

  private onWheelUpActionPrefChanged_(
    actions: ChainedActionEntry[] | undefined) {
    this.wheelUpRow_ = this.buildRockerWheelRow_(
      GESTURES_WHEEL_UP_PREF, actions, GestureCommand.PREVIOUS_TAB)
  }

  private onWheelDownActionPrefChanged_(
    actions: ChainedActionEntry[] | undefined) {
    this.wheelDownRow_ = this.buildRockerWheelRow_(
      GESTURES_WHEEL_DOWN_PREF, actions, GestureCommand.NEXT_TAB)
  }

  // All 4 rocker/wheel rows are each their own standalone list pref (not an
  // entry in a shared dict/array like mappings/custom gestures), so a single
  // writer parameterized by `prefName` covers all of them.
  private writeRockerWheelActions_(
    prefName: string, gestureCommand: GestureCommand, navigateToUrl: string,
    chainedActions: ChainedActionRow[]) {
    this.setPrefValue(
      prefName,
      buildActionEntries({ gestureCommand, navigateToUrl }, chainedActions))
  }

  // The 4 rows share identical behavior, so a `data-pref` attribute on each
  // row's container (set to its `prefName`) identifies which one dispatched,
  // rather than needing 4 near-duplicate handlers per interaction.
  private getRockerWheelRow_(prefName: string): RockerWheelRow {
    switch (prefName) {
      case GESTURES_ROCK_LEFT_PREF: return this.rockLeftRow_
      case GESTURES_ROCK_RIGHT_PREF: return this.rockRightRow_
      case GESTURES_WHEEL_UP_PREF: return this.wheelUpRow_
      case GESTURES_WHEEL_DOWN_PREF: return this.wheelDownRow_
      default: throw new Error(`Unknown rocker/wheel pref: ${prefName}`)
    }
  }

  private getRockerWheelPref_(e: Event): string {
    return (e.currentTarget as HTMLElement).closest<HTMLElement>(
      '[data-pref]')!.dataset['pref']!
  }

  private onRockerWheelCommandChanged_(e: Event) {
    const prefName = this.getRockerWheelPref_(e)
    const row = this.getRockerWheelRow_(prefName)
    this.writeRockerWheelActions_(
      prefName, Number((e.target as HTMLSelectElement).value) as
        GestureCommand, row.navigateToUrl, row.chainedActions)
  }

  private onWheelModifierButtonChanged_(e: Event) {
    this.setPrefValue(
      GESTURES_WHEEL_MODIFIER_BUTTON_PREF,
      Number((e.target as HTMLSelectElement).value))
  }

  private onWheelSensitivityChanged_(e: Event) {
    const input = e.target as HTMLInputElement
    const value = Number(input.value)
    // Snap to the default when released close to it, so it's easy to land
    // back on exactly the out-of-the-box sensitivity.
    const snapped =
      Math.abs(value - DEFAULT_WHEEL_SENSITIVITY_IN_PIXELS) <=
          WHEEL_SENSITIVITY_SNAP_THRESHOLD_IN_PIXELS
        ? DEFAULT_WHEEL_SENSITIVITY_IN_PIXELS
        : value
    this.setPrefValue(GESTURES_WHEEL_SENSITIVITY_IN_PIXELS_PREF, snapped)
  }

  private onEditRockerWheelUrlClick_(e: Event) {
    const row = this.getRockerWheelRow_(this.getRockerWheelPref_(e))
    this.urlDialogContext_ =
      { rockerWheelRow: row, gestureCommand: row.gestureCommand }
    this.urlDialogInput_ = row.navigateToUrl
    this.showUrlDialog_ = true
  }

  private onAddRockerWheelChainedActionClick_(e: Event) {
    // Rebuilding the row below replaces this row's DOM; blur first so focus
    // settles on the body instead of wherever it lands afterward.
    ;(e.target as HTMLElement).blur()
    const prefName = this.getRockerWheelPref_(e)
    const row = this.getRockerWheelRow_(prefName)
    this.writeRockerWheelActions_(
      prefName, row.gestureCommand, row.navigateToUrl, [
        ...row.chainedActions,
        { gestureCommand: GestureCommand.UNASSIGNED, navigateToUrl: '' },
      ])
  }

  // Only shown when there's at least one chained action, so there's always
  // a next action to promote into the primary slot this one vacates.
  private onRemoveRockerWheelPrimaryActionClick_(e: Event) {
    const prefName = this.getRockerWheelPref_(e)
    const row = this.getRockerWheelRow_(prefName)
    const [newPrimary, ...rest] = row.chainedActions
    this.writeRockerWheelActions_(
      prefName, newPrimary.gestureCommand, newPrimary.navigateToUrl, rest)
  }

  private onRockerWheelChainedActionCommandChanged_(e: Event &
    { model: { chainedAction: ChainedActionRow } }) {
    const prefName = this.getRockerWheelPref_(e)
    const row = this.getRockerWheelRow_(prefName)
    const gestureCommand =
      Number((e.target as HTMLSelectElement).value) as GestureCommand
    this.writeRockerWheelActions_(
      prefName, row.gestureCommand, row.navigateToUrl, row.chainedActions.map(
        (action) => action === e.model.chainedAction
          ? { ...action, gestureCommand }
          : action))
  }

  private onRemoveRockerWheelChainedActionClick_(e: Event &
    { model: { chainedAction: ChainedActionRow } }) {
    const prefName = this.getRockerWheelPref_(e)
    const row = this.getRockerWheelRow_(prefName)
    this.writeRockerWheelActions_(
      prefName, row.gestureCommand, row.navigateToUrl,
      row.chainedActions.filter(
        (action) => action !== e.model.chainedAction))
  }

  private onEditRockerWheelChainedUrlClick_(e: Event &
    { model: { chainedAction: ChainedActionRow } }) {
    const row = this.getRockerWheelRow_(this.getRockerWheelPref_(e))
    this.urlDialogContext_ = {
      rockerWheelRow: row,
      chainedAction: e.model.chainedAction,
      gestureCommand: e.model.chainedAction.gestureCommand,
    }
    this.urlDialogInput_ = e.model.chainedAction.navigateToUrl
    this.showUrlDialog_ = true
  }

  private onRockerWheelPrimaryActionDragStart_(e: DragEvent) {
    this.dragContext_ = {
      rockerWheelRow: this.getRockerWheelRow_(this.getRockerWheelPref_(e)),
      draggedAction: null,
    }
    e.dataTransfer!.effectAllowed = 'move'
    e.dataTransfer!.setData('text/plain', '')
  }

  private onRockerWheelPrimaryActionDrop_(e: DragEvent) {
    e.preventDefault()
    const prefName = this.getRockerWheelPref_(e)
    const row = this.getRockerWheelRow_(prefName)
    const context = this.dragContext_
    this.dragContext_ = null
    if (!context?.rockerWheelRow ||
        context.rockerWheelRow.prefName !== prefName ||
        context.draggedAction === null) {
      return
    }
    const swapped = this.swapPrimaryWithChainedAction_(
      row, row.chainedActions, context.draggedAction)
    this.writeRockerWheelActions_(
      prefName, swapped.primary.gestureCommand, swapped.primary.navigateToUrl,
      swapped.chainedActions)
  }

  private onRockerWheelChainedActionDragStart_(e: DragEvent &
    { model: { chainedAction: ChainedActionRow } }) {
    this.dragContext_ = {
      rockerWheelRow: this.getRockerWheelRow_(this.getRockerWheelPref_(e)),
      draggedAction: e.model.chainedAction,
    }
    e.dataTransfer!.effectAllowed = 'move'
    e.dataTransfer!.setData('text/plain', '')
  }

  private onRockerWheelChainedActionDrop_(e: DragEvent &
    { model: { chainedAction: ChainedActionRow } }) {
    e.preventDefault()
    const prefName = this.getRockerWheelPref_(e)
    const row = this.getRockerWheelRow_(prefName)
    const context = this.dragContext_
    this.dragContext_ = null
    if (!context?.rockerWheelRow ||
        context.rockerWheelRow.prefName !== prefName) {
      return
    }
    if (context.draggedAction === null) {
      const swapped = this.swapPrimaryWithChainedAction_(
        row, row.chainedActions, e.model.chainedAction)
      this.writeRockerWheelActions_(
        prefName, swapped.primary.gestureCommand,
        swapped.primary.navigateToUrl, swapped.chainedActions)
      return
    }
    this.writeRockerWheelActions_(
      prefName, row.gestureCommand, row.navigateToUrl,
      this.reorderChainedActions_(
        row.chainedActions, context.draggedAction, e.model.chainedAction))
  }

  // True for any command whose destination/pattern is edited via the
  // shared URL/pattern dialog (see `urlDialogContext_`'s own comment).
  private hasTextParam_(gestureCommand: GestureCommand): boolean {
    return gestureCommand === GestureCommand.NAVIGATE_TO_URL ||
      gestureCommand === GestureCommand.SWITCH_TO_TAB
  }

  private getCommandName_(gestureCommand: GestureCommand): string {
    return this.i18n(COMMAND_I18N_KEYS[gestureCommand])
  }

  private getCommandSelectClass_(
      gestureCommand: GestureCommand, navigateToUrl: string): string {
    const classes = ['md-select']
    if (gestureCommand === GestureCommand.UNASSIGNED) {
      classes.push('command-unassigned')
    }
    if (this.isActionIncomplete_(gestureCommand, navigateToUrl)) {
      classes.push('action-incomplete')
    }
    return classes.join(' ')
  }

  // An action with no command set, or one needing a URL/pattern not set
  // yet, won't do anything if the gesture runs right now.
  private isActionIncomplete_(
      gestureCommand: GestureCommand, navigateToUrl: string): boolean {
    return gestureCommand === GestureCommand.UNASSIGNED ||
      (this.hasTextParam_(gestureCommand) && !navigateToUrl)
  }

  private getUrlAttachClass_(navigateToUrl: string): string {
    return navigateToUrl ? 'param-attach' : 'param-attach action-incomplete'
  }

  // Native `<option>` has no one-way `value` binding of its own; the
  // `selected` attribute on each option is how the `<select>`'s current
  // value is reflected.
  private optionMatchesCommand_(
      optionValue: GestureCommand, gestureCommand: GestureCommand): boolean {
    return optionValue === gestureCommand
  }

  private onCommandChanged_(
    e: DomRepeatEvent<MappingRow>) {
    const row = e.model.item
    this.reassignCommand_(
      Number((e.target as HTMLSelectElement).value) as GestureCommand,
      { mappingKey: row.key })
  }

  private onCustomGestureCommandChanged_(
    e: DomRepeatEvent<CustomGestureRow>) {
    const row = e.model.item
    this.reassignCommand_(
      Number((e.target as HTMLSelectElement).value) as GestureCommand,
      { customGestureId: row.id })
  }

  private onEditMappingUrlClick_(e: DomRepeatEvent<MappingRow>) {
    const row = e.model.item
    this.urlDialogContext_ = { mappingRow: row, gestureCommand: row.gestureCommand }
    this.urlDialogInput_ = row.navigateToUrl
    this.showUrlDialog_ = true
  }

  private onEditCustomGestureUrlClick_(e: DomRepeatEvent<CustomGestureRow>) {
    const row = e.model.item
    this.urlDialogContext_ =
      { customGestureRow: row, gestureCommand: row.gestureCommand }
    this.urlDialogInput_ = row.navigateToUrl
    this.showUrlDialog_ = true
  }

  // Only shown when there's at least one chained action, so there's always
  // a next action to promote into the primary slot this one vacates.
  private onRemoveMappingPrimaryActionClick_(e: DomRepeatEvent<MappingRow>) {
    const row = e.model.item
    const [newPrimary, ...rest] = row.chainedActions
    this.writeMappingActions_(
      row.key, newPrimary.gestureCommand, newPrimary.navigateToUrl, rest)
  }

  private onRemoveCustomGesturePrimaryActionClick_(
    e: DomRepeatEvent<CustomGestureRow>) {
    const row = e.model.item
    const [newPrimary, ...rest] = row.chainedActions
    this.writeCustomGestureActions_(
      row.id, newPrimary.gestureCommand, newPrimary.navigateToUrl, rest)
  }

  private onUrlDialogInputChanged_(e: Event) {
    this.urlDialogInput_ = (e.target as HTMLInputElement).value
  }

  private onUrlDialogClose_() {
    this.showUrlDialog_ = false
    this.urlDialogContext_ = null
  }

  private onConfirmUrlDialogClick_() {
    const context = this.urlDialogContext_
    if (!context) {
      return
    }
    // A Switch to Tab pattern is matched literally (see
    // `MatchesGlobPattern`), not navigated to, so it shouldn't get an
    // `https://` scheme prepended the way a URL does.
    const input = this.urlDialogInput_.trim()
    const url = context.gestureCommand === GestureCommand.SWITCH_TO_TAB
      ? input : withUrlScheme(input)
    if (context.mappingRow) {
      const row = context.mappingRow
      this.writeMappingActions_(
        row.key, row.gestureCommand,
        context.chainedAction ? row.navigateToUrl : url,
        context.chainedAction
          ? row.chainedActions.map((action) => action === context.chainedAction
            ? { ...action, navigateToUrl: url }
            : action)
          : row.chainedActions)
    } else if (context.customGestureRow) {
      const row = context.customGestureRow
      this.writeCustomGestureActions_(
        row.id, row.gestureCommand,
        context.chainedAction ? row.navigateToUrl : url,
        context.chainedAction
          ? row.chainedActions.map((action) => action === context.chainedAction
            ? { ...action, navigateToUrl: url }
            : action)
          : row.chainedActions)
    } else if (context.isCapture) {
      if (context.chainedAction) {
        this.capturedChainedActions_ = this.capturedChainedActions_.map(
          (action) => action === context.chainedAction
            ? { ...action, navigateToUrl: url }
            : action)
      } else {
        this.selectedCaptureNavigateToUrl_ = url
      }
    } else if (context.rockerWheelRow) {
      const row = context.rockerWheelRow
      this.writeRockerWheelActions_(
        row.prefName, row.gestureCommand,
        context.chainedAction ? row.navigateToUrl : url,
        context.chainedAction
          ? row.chainedActions.map((action) => action === context.chainedAction
            ? { ...action, navigateToUrl: url }
            : action)
          : row.chainedActions)
    }
    this.showUrlDialog_ = false
    this.urlDialogContext_ = null
  }

  // Displayed on the attached URL/pattern button; the chosen value once
  // set, with its scheme stripped for a URL (always http/https) and left
  // as-is for a pattern, or a placeholder prompting the user to set one.
  // Display only; the stored value is unchanged.
  private getUrlButtonLabel_(
      gestureCommand: GestureCommand, navigateToUrl: string): string {
    if (navigateToUrl) {
      return gestureCommand === GestureCommand.SWITCH_TO_TAB
        ? navigateToUrl : navigateToUrl.replace(/^https?:\/\//, '')
    }
    return this.i18n(gestureCommand === GestureCommand.SWITCH_TO_TAB
      ? 'braveGesturesSetPattern' : 'braveGesturesSetUrl')
  }

  // Title/placeholder for the shared URL/pattern dialog (see
  // `urlDialogContext_`'s own comment), based on which command is being
  // edited.
  private urlDialogTitle_(): string {
    return this.i18n(
      this.urlDialogContext_?.gestureCommand === GestureCommand.SWITCH_TO_TAB
        ? 'braveGesturesCommandSwitchToTab'
        : 'braveGesturesCommandNavigateToUrl')
  }

  private urlDialogPlaceholder_(): string {
    return this.i18n(
      this.urlDialogContext_?.gestureCommand === GestureCommand.SWITCH_TO_TAB
        ? 'braveGesturesSwitchToTabPlaceholder'
        : 'braveGesturesNavigateToUrlPlaceholder')
  }

  private writeMappingActions_(
    key: string, gestureCommand: GestureCommand, navigateToUrl: string,
    chainedActions: ChainedActionRow[]) {
    this.setPrefDictEntry(
      GESTURES_MAPPINGS_PREF, key,
      buildActionEntries({ gestureCommand, navigateToUrl }, chainedActions))
  }

  private existingCustomGestures_(): CustomGestureEntry[] {
    return this.getPref<CustomGestureEntry[]>(
      GESTURES_CUSTOM_GESTURES_PREF).value ?? []
  }

  private writeCustomGestureActions_(
    id: string, gestureCommand: GestureCommand, navigateToUrl: string,
    chainedActions: ChainedActionRow[]) {
    const existingCustomGestures = this.existingCustomGestures_()
    this.setPrefValue(
      GESTURES_CUSTOM_GESTURES_PREF,
      existingCustomGestures.map((customGesture) => customGesture.id === id
        ? {
          ...customGesture,
          actions: buildActionEntries(
            { gestureCommand, navigateToUrl }, chainedActions),
        }
        : customGesture))
  }

  private onAddMappingChainedActionClick_(e: DomRepeatEvent<MappingRow>) {
    // Rebuilding mappingRows_ below replaces this row's DOM; blur first so
    // focus settles on the body instead of wherever it lands afterward.
    ;(e.target as HTMLElement).blur()
    const row = e.model.item
    this.writeMappingActions_(row.key, row.gestureCommand, row.navigateToUrl, [
      ...row.chainedActions,
      { gestureCommand: GestureCommand.UNASSIGNED, navigateToUrl: '' },
    ])
  }

  private onRemoveMappingChainedActionClick_(e: Event &
    { model: { chainedAction: ChainedActionRow,
               parentModel: { item: MappingRow } } }) {
    const row = e.model.parentModel.item
    this.writeMappingActions_(
      row.key, row.gestureCommand, row.navigateToUrl,
      row.chainedActions.filter(
        (action) => action !== e.model.chainedAction))
  }

  private onMappingChainedActionCommandChanged_(e: Event &
    { model: { chainedAction: ChainedActionRow,
               parentModel: { item: MappingRow } } }) {
    const row = e.model.parentModel.item
    const gestureCommand =
      Number((e.target as HTMLSelectElement).value) as GestureCommand
    this.writeMappingActions_(
      row.key, row.gestureCommand, row.navigateToUrl, row.chainedActions.map(
        (action) => action === e.model.chainedAction
          ? { ...action, gestureCommand }
          : action))
  }

  private onEditMappingChainedUrlClick_(e: Event &
    { model: { chainedAction: ChainedActionRow,
               parentModel: { item: MappingRow } } }) {
    this.urlDialogContext_ = {
      mappingRow: e.model.parentModel.item,
      chainedAction: e.model.chainedAction,
      gestureCommand: e.model.chainedAction.gestureCommand,
    }
    this.urlDialogInput_ = e.model.chainedAction.navigateToUrl
    this.showUrlDialog_ = true
  }

  // Allows a drop; without this, the browser rejects the drop entirely.
  // Shared across mapping, custom gesture, and capture chained actions,
  // since it doesn't need to know which one is being dragged. Deliberately
  // does nothing else: two different "update the view mid-drag" approaches
  // (reassigning the bound array, and a dom-repeat `sort`-based preview)
  // both reproduced a browser-process crash on tab close after a drag, so
  // the reorder only ever happens once, in the drop handler, after the
  // native drag session has fully ended. Do not add live preview behavior
  // here without first confirming it doesn't reintroduce that crash.
  private onChainedActionDragOver_(e: DragEvent) {
    e.preventDefault()
    if (e.dataTransfer) {
      e.dataTransfer.dropEffect = 'move'
    }
  }

  // Clears a drag that didn't end in a drop (e.g. released outside any
  // valid target). A drop also clears dragContext_ itself, so this is a
  // no-op in that case.
  private onChainedActionDragEnd_() {
    this.dragContext_ = null
  }

  // Moves `dragged` to `target`'s slot: after it when `dragged` started
  // earlier in the list, before it when `dragged` started later. Using a
  // fixed "always insert before" rule would make dropping onto the very
  // next item a no-op, since that's already its position relative to
  // `dragged`.
  private reorderChainedActions_(
      chainedActions: ChainedActionRow[], dragged: ChainedActionRow,
      target: ChainedActionRow): ChainedActionRow[] {
    if (dragged === target) {
      return chainedActions
    }
    const movingDown =
      chainedActions.indexOf(dragged) < chainedActions.indexOf(target)
    const withoutDragged = chainedActions.filter((action) => action !== dragged)
    const targetIndex = withoutDragged.indexOf(target)
    const insertAt = movingDown ? targetIndex + 1 : targetIndex
    return [
      ...withoutDragged.slice(0, insertAt),
      dragged,
      ...withoutDragged.slice(insertAt),
    ]
  }

  // Promotes `chainedAction` into the primary slot, demoting the current
  // primary into `chainedAction`'s old slot.
  private swapPrimaryWithChainedAction_(
      primary: { gestureCommand: GestureCommand, navigateToUrl: string },
      chainedActions: ChainedActionRow[], chainedAction: ChainedActionRow):
      { primary: ChainedActionRow, chainedActions: ChainedActionRow[] } {
    return {
      primary: {
        gestureCommand: chainedAction.gestureCommand,
        navigateToUrl: chainedAction.navigateToUrl,
      },
      chainedActions: chainedActions.map((action) => action === chainedAction
        ? { gestureCommand: primary.gestureCommand,
            navigateToUrl: primary.navigateToUrl }
        : action),
    }
  }

  private onMappingChainedActionDragStart_(e: DragEvent &
    { model: { chainedAction: ChainedActionRow,
               parentModel: { item: MappingRow } } }) {
    this.dragContext_ = {
      mappingRow: e.model.parentModel.item,
      draggedAction: e.model.chainedAction,
    }
    e.dataTransfer!.effectAllowed = 'move'
    // Some drag implementations only treat the drag as valid, and fire
    // `drop`, once `dataTransfer` actually carries data.
    e.dataTransfer!.setData('text/plain', '')
  }

  private onMappingChainedActionDrop_(e: DragEvent &
    { model: { chainedAction: ChainedActionRow,
               parentModel: { item: MappingRow } } }) {
    e.preventDefault()
    const context = this.dragContext_
    const row = e.model.parentModel.item
    this.dragContext_ = null
    if (!context?.mappingRow || context.mappingRow !== row) {
      return
    }
    if (context.draggedAction === null) {
      const swapped = this.swapPrimaryWithChainedAction_(
        row, row.chainedActions, e.model.chainedAction)
      this.writeMappingActions_(
        row.key, swapped.primary.gestureCommand,
        swapped.primary.navigateToUrl, swapped.chainedActions)
      return
    }
    this.writeMappingActions_(
      row.key, row.gestureCommand, row.navigateToUrl,
      this.reorderChainedActions_(
        row.chainedActions, context.draggedAction, e.model.chainedAction))
  }

  private onMappingPrimaryActionDragStart_(
      e: DragEvent & { model: { item: MappingRow } }) {
    this.dragContext_ = { mappingRow: e.model.item, draggedAction: null }
    e.dataTransfer!.effectAllowed = 'move'
    e.dataTransfer!.setData('text/plain', '')
  }

  private onMappingPrimaryActionDrop_(
      e: DragEvent & { model: { item: MappingRow } }) {
    e.preventDefault()
    const context = this.dragContext_
    const row = e.model.item
    this.dragContext_ = null
    if (!context?.mappingRow || context.mappingRow !== row ||
        context.draggedAction === null) {
      return
    }
    const swapped = this.swapPrimaryWithChainedAction_(
      row, row.chainedActions, context.draggedAction)
    this.writeMappingActions_(
      row.key, swapped.primary.gestureCommand, swapped.primary.navigateToUrl,
      swapped.chainedActions)
  }

  private onAddCustomGestureChainedActionClick_(
    e: DomRepeatEvent<CustomGestureRow>) {
    // Rebuilding customGestureRows_ below replaces this row's DOM; blur
    // first so focus settles on the body instead of wherever it lands
    // afterward.
    ;(e.target as HTMLElement).blur()
    const row = e.model.item
    this.writeCustomGestureActions_(
      row.id, row.gestureCommand, row.navigateToUrl, [
        ...row.chainedActions,
        { gestureCommand: GestureCommand.UNASSIGNED, navigateToUrl: '' },
      ])
  }

  private onRemoveCustomGestureChainedActionClick_(e: Event &
    { model: { chainedAction: ChainedActionRow,
               parentModel: { item: CustomGestureRow } } }) {
    const row = e.model.parentModel.item
    this.writeCustomGestureActions_(
      row.id, row.gestureCommand, row.navigateToUrl,
      row.chainedActions.filter(
        (action) => action !== e.model.chainedAction))
  }

  private onCustomGestureChainedActionCommandChanged_(e: Event &
    { model: { chainedAction: ChainedActionRow,
               parentModel: { item: CustomGestureRow } } }) {
    const row = e.model.parentModel.item
    const gestureCommand =
      Number((e.target as HTMLSelectElement).value) as GestureCommand
    this.writeCustomGestureActions_(
      row.id, row.gestureCommand, row.navigateToUrl, row.chainedActions.map(
        (action) => action === e.model.chainedAction
          ? { ...action, gestureCommand }
          : action))
  }

  private onEditCustomGestureChainedUrlClick_(e: Event &
    { model: { chainedAction: ChainedActionRow,
               parentModel: { item: CustomGestureRow } } }) {
    this.urlDialogContext_ = {
      customGestureRow: e.model.parentModel.item,
      chainedAction: e.model.chainedAction,
      gestureCommand: e.model.chainedAction.gestureCommand,
    }
    this.urlDialogInput_ = e.model.chainedAction.navigateToUrl
    this.showUrlDialog_ = true
  }

  private onCustomGestureChainedActionDragStart_(e: DragEvent &
    { model: { chainedAction: ChainedActionRow,
               parentModel: { item: CustomGestureRow } } }) {
    this.dragContext_ = {
      customGestureRow: e.model.parentModel.item,
      draggedAction: e.model.chainedAction,
    }
    e.dataTransfer!.effectAllowed = 'move'
    // Some drag implementations only treat the drag as valid, and fire
    // `drop`, once `dataTransfer` actually carries data.
    e.dataTransfer!.setData('text/plain', '')
  }

  private onCustomGestureChainedActionDrop_(e: DragEvent &
    { model: { chainedAction: ChainedActionRow,
               parentModel: { item: CustomGestureRow } } }) {
    e.preventDefault()
    const context = this.dragContext_
    const row = e.model.parentModel.item
    this.dragContext_ = null
    if (!context?.customGestureRow || context.customGestureRow !== row) {
      return
    }
    if (context.draggedAction === null) {
      const swapped = this.swapPrimaryWithChainedAction_(
        row, row.chainedActions, e.model.chainedAction)
      this.writeCustomGestureActions_(
        row.id, swapped.primary.gestureCommand,
        swapped.primary.navigateToUrl, swapped.chainedActions)
      return
    }
    this.writeCustomGestureActions_(
      row.id, row.gestureCommand, row.navigateToUrl,
      this.reorderChainedActions_(
        row.chainedActions, context.draggedAction, e.model.chainedAction))
  }

  private onCustomGesturePrimaryActionDragStart_(
      e: DragEvent & { model: { item: CustomGestureRow } }) {
    this.dragContext_ = { customGestureRow: e.model.item, draggedAction: null }
    e.dataTransfer!.effectAllowed = 'move'
    e.dataTransfer!.setData('text/plain', '')
  }

  private onCustomGesturePrimaryActionDrop_(
      e: DragEvent & { model: { item: CustomGestureRow } }) {
    e.preventDefault()
    const context = this.dragContext_
    const row = e.model.item
    this.dragContext_ = null
    if (!context?.customGestureRow || context.customGestureRow !== row ||
        context.draggedAction === null) {
      return
    }
    const swapped = this.swapPrimaryWithChainedAction_(
      row, row.chainedActions, context.draggedAction)
    this.writeCustomGestureActions_(
      row.id, swapped.primary.gestureCommand, swapped.primary.navigateToUrl,
      swapped.chainedActions)
  }

  private rowCommandSequence_(
      row: { gestureCommand: GestureCommand,
             chainedActions: ChainedActionRow[] }): GestureCommand[] {
    return [row.gestureCommand,
      ...row.chainedActions.map((action) => action.gestureCommand)]
  }

  // Returns the other gestures whose full ordered command sequence matches
  // `commands` exactly, excluding `target` itself. A gesture chained
  // differently (even if it shares the same first command) isn't a
  // conflict. `UNASSIGNED` as the first command is the default, unbound
  // state that many gestures legitimately share, so it's never treated as
  // a conflict.
  private findConflictingGestures_(
      commands: GestureCommand[],
    target: { mappingKey?: string, customGestureId?: string }): { keys: string[], customGestureIds: string[] } {
    if (commands[0] === GestureCommand.UNASSIGNED) {
      return { keys: [], customGestureIds: [] }
    }
    const matches = (sequence: GestureCommand[]) =>
      sequence.length === commands.length &&
      sequence.every((command, i) => command === commands[i])
    return {
      keys: this.mappingRows_
        .filter((row) =>
          row.key !== target.mappingKey &&
          matches(this.rowCommandSequence_(row)))
        .map((row) => row.key),
      customGestureIds: this.customGestureRows_
        .filter((row) =>
          row.id !== target.customGestureId &&
          matches(this.rowCommandSequence_(row)))
        .map((row) => row.id),
    }
  }

  // Sets `gestureCommand` on the gesture identified by `target`, either a
  // fixed direction-key mapping via `mappingKey` or a custom gesture via
  // `customGestureId`. Both gestures are allowed to share the same action;
  // the clashing rows are flagged instead (see `mappingRowClashClass_` and
  // `customGestureRowClashClass_`), nothing is cleared or blocked.
  private reassignCommand_(
      gestureCommand: GestureCommand,
    target: { mappingKey?: string, customGestureId?: string }) {
    this.applyCommand_(gestureCommand, target)
  }

  private applyCommand_(
      gestureCommand: GestureCommand,
    target: { mappingKey?: string, customGestureId?: string }) {
    if (target.mappingKey !== undefined) {
      const row = this.mappingRows_.find((r) => r.key === target.mappingKey)
      this.writeMappingActions_(
        target.mappingKey, gestureCommand, row?.navigateToUrl ?? '',
        row?.chainedActions ?? [])
    }
    if (target.customGestureId !== undefined) {
      const row =
        this.customGestureRows_.find((r) => r.id === target.customGestureId)
      this.writeCustomGestureActions_(
        target.customGestureId, gestureCommand, row?.navigateToUrl ?? '',
        row?.chainedActions ?? [])
    }
  }

  // Highlights a mapping whose full command sequence is also used by
  // another gesture or custom gesture, or that a custom gesture's drawn
  // path would resolve to instead of running as a custom gesture. Takes
  // `mappingRows_` and `customGestureRows_` as explicit, otherwise-unused
  // parameters so this recomputes whenever either changes; a clash with a
  // custom gesture otherwise wouldn't be re-checked when only
  // `customGestureRows_` changes, since Polymer only tracks `item` as this
  // binding's dependency.
  private mappingRowClashClass_(
      row: MappingRow, _mappingRows: MappingRow[],
      _customGestureRows: CustomGestureRow[]): string {
    const { keys, customGestureIds } = this.findConflictingGestures_(
      this.rowCommandSequence_(row), { mappingKey: row.key })
    const hasClash = keys.length > 0 || customGestureIds.length > 0 ||
      this.anyCustomGestureMatchesDirectionalKey_(row.key)
    return hasClash ? 'command-clash' : ''
  }

  // True if any saved custom gesture's drawn path would resolve to the
  // directional mapping keyed by `key` instead of running as a custom
  // gesture, making that custom gesture unreachable.
  private anyCustomGestureMatchesDirectionalKey_(key: string): boolean {
    const existingCustomGestures = this.existingCustomGestures_()
    const minStrokePixels = this.getPref<number>(
      GESTURES_SENSITIVITY_IN_PIXELS_PREF).value ?? DEFAULT_SENSITIVITY_IN_PIXELS
    return existingCustomGestures.some((gesture) => classifyDirectionalStrokeKey(
      gesture.points, minStrokePixels, MAX_DIRECTIONAL_STROKES) === key)
  }

  // Highlights a custom gesture whose full command sequence is also used by
  // another gesture or custom gesture, or whose drawn path looks similar
  // enough to another custom gesture's that the recognizer could confuse
  // them. See `mappingRowClashClass_` for why both arrays are taken
  // explicitly.
  private customGestureRowClashClass_(
      row: CustomGestureRow, _mappingRows: MappingRow[],
      _customGestureRows: CustomGestureRow[]): string {
    const { keys, customGestureIds } = this.findConflictingGestures_(
      this.rowCommandSequence_(row), { customGestureId: row.id })
    const hasClash = keys.length > 0 || customGestureIds.length > 0 ||
      this.shapeClashDescription_(row.id) !== ''
    return hasClash ? 'command-clash' : ''
  }

  // Describes why this custom gesture's drawn path clashes with another's,
  // or '' if it doesn't: either it would resolve to a mapped directional
  // gesture instead, which always wins over a custom gesture at dispatch
  // time and makes this one unreachable, or it's similar enough to another
  // custom gesture's that the recognizer could confuse them (the same
  // check as the live-capture similarity warning). Independent of whether
  // their actions also clash.
  private shapeClashDescription_(customGestureId: string): string {
    const existingCustomGestures = this.existingCustomGestures_()
    const entry =
      existingCustomGestures.find((e) => e.id === customGestureId)
    if (!entry) {
      return ''
    }
    if (this.matchesMappedDirectionalGesture_(entry.points)) {
      return this.i18n('braveGesturesShapeClashDirectionalSublabel')
    }
    const others =
      existingCustomGestures.filter((e) => e.id !== customGestureId)
    const distance = closestCustomGestureDistance(entry.points, others)
    if (distance !== null &&
      distance < CUSTOM_GESTURE_SIMILARITY_WARNING_THRESHOLD) {
      return this.i18n('braveGesturesShapeClashSublabel')
    }
    return ''
  }

  // True if `points`, drawn as a custom gesture, would actually resolve to
  // a mapped directional gesture instead: `FinishDrawing` always tries
  // directional-stroke matching before falling back to custom gestures, so
  // a custom gesture whose path classifies to a mapped stroke key would
  // never actually run.
  private matchesMappedDirectionalGesture_(
      points: CustomGesturePoint[]): boolean {
    const minStrokePixels = this.getPref<number>(
      GESTURES_SENSITIVITY_IN_PIXELS_PREF).value ?? DEFAULT_SENSITIVITY_IN_PIXELS
    const key = classifyDirectionalStrokeKey(
      points, minStrokePixels, MAX_DIRECTIONAL_STROKES)
    if (!key) {
      return false
    }
    const row = this.mappingRows_.find((r) => r.key === key)
    return row !== undefined &&
      row.gestureCommand !== GestureCommand.UNASSIGNED
  }

  // Names which other gesture(s)/custom gesture(s) a clash is against.
  // Mapping keys (e.g. "←", "↓→") are shown as-is; custom gestures have no
  // readable name, so they're just counted.
  private clashDescription_(keys: string[], customGestureIds: string[]):
      string {
    if (!keys.length && !customGestureIds.length) {
      return ''
    }
    const parts = [...keys]
    if (customGestureIds.length === 1) {
      parts.push(this.i18n('braveGesturesClashCustomGestureSingular'))
    } else if (customGestureIds.length > 1) {
      parts.push(this.i18n(
        'braveGesturesClashCustomGesturePlural', customGestureIds.length))
    }
    return this.i18n('braveGesturesCommandClashSublabel', parts.join(', '))
  }

  private mappingClashDescription_(
      row: MappingRow, _mappingRows: MappingRow[],
      _customGestureRows: CustomGestureRow[]): string {
    const { keys, customGestureIds } = this.findConflictingGestures_(
      this.rowCommandSequence_(row), { mappingKey: row.key })
    const commandClash = this.clashDescription_(keys, customGestureIds)
    if (!this.anyCustomGestureMatchesDirectionalKey_(row.key)) {
      return commandClash
    }
    const shapeClash = this.i18n('braveGesturesShapeClashDirectionalSublabel')
    return commandClash ? `${commandClash}. ${shapeClash}` : shapeClash
  }

  private customGestureClashDescription_(
      row: CustomGestureRow, _mappingRows: MappingRow[],
      _customGestureRows: CustomGestureRow[]): string {
    const { keys, customGestureIds } = this.findConflictingGestures_(
      this.rowCommandSequence_(row), { customGestureId: row.id })
    const commandClash = this.clashDescription_(keys, customGestureIds)
    const shapeClash = this.shapeClashDescription_(row.id)
    if (!shapeClash) {
      return commandClash
    }
    return commandClash ? `${commandClash}. ${shapeClash}` : shapeClash
  }

  private onResetMappingsClick_() {
    this.showResetConfirmDialog_ = true
  }

  private onResetDialogClose_() {
    this.showResetConfirmDialog_ = false
  }

  private onConfirmResetClick_() {
    for (const row of this.mappingRows_) {
      this.deletePrefDictEntry(GESTURES_MAPPINGS_PREF, row.key)
    }
    this.showResetConfirmDialog_ = false
  }

  private onResetRockerClick_() {
    this.showRockerResetConfirmDialog_ = true
  }

  private onRockerResetDialogClose_() {
    this.showRockerResetConfirmDialog_ = false
  }

  // Each row is its own standalone list pref rather than a dict entry, so
  // resetting means writing each one back to its single-action default,
  // matching `prefs_registration.cc`'s registered defaults.
  private onConfirmRockerResetClick_() {
    this.writeRockerWheelActions_(
      GESTURES_ROCK_LEFT_PREF, GestureCommand.BACK, '', [])
    this.writeRockerWheelActions_(
      GESTURES_ROCK_RIGHT_PREF, GestureCommand.FORWARD, '', [])
    this.showRockerResetConfirmDialog_ = false
  }

  private onResetWheelClick_() {
    this.showWheelResetConfirmDialog_ = true
  }

  private onWheelResetDialogClose_() {
    this.showWheelResetConfirmDialog_ = false
  }

  // Also resets the modifier button and sensitivity, unlike
  // `onConfirmRockerResetClick_`: wheel gestures have no fixed left/right
  // mapping the way rocker gestures do, so which button modifies the wheel
  // and how far it must scroll are themselves configurable, with their own
  // registered defaults in `prefs_registration.cc`.
  private onConfirmWheelResetClick_() {
    this.writeRockerWheelActions_(
      GESTURES_WHEEL_UP_PREF, GestureCommand.PREVIOUS_TAB, '', [])
    this.writeRockerWheelActions_(
      GESTURES_WHEEL_DOWN_PREF, GestureCommand.NEXT_TAB, '', [])
    this.setPrefValue(
      GESTURES_WHEEL_MODIFIER_BUTTON_PREF, DEFAULT_WHEEL_MODIFIER_BUTTON)
    this.setPrefValue(
      GESTURES_WHEEL_SENSITIVITY_IN_PIXELS_PREF,
      DEFAULT_WHEEL_SENSITIVITY_IN_PIXELS)
    this.showWheelResetConfirmDialog_ = false
  }

  private onAddSiteClick_() {
    this.editingSiteOrigin_ = null
    this.isEditingSite_ = false
    this.addSiteInput_ = ''
    this.showAddSiteDialog_ = true
  }

  private onEditSiteClick_(e: DomRepeatEvent<DisallowedSiteRow>) {
    const row = e.model.item
    this.editingSiteOrigin_ = row.origin
    this.isEditingSite_ = true
    this.addSiteInput_ = row.origin
    this.showAddSiteDialog_ = true
  }

  private getAddSiteDialogTitle_(isEditingSite: boolean): string {
    return isEditingSite
      ? this.i18n('braveGesturesEditSite')
      : this.i18n('braveGesturesAddSite')
  }

  private onAddSiteDialogClose_() {
    this.showAddSiteDialog_ = false
  }

  private onAddSiteInputChanged_(e: Event) {
    this.addSiteInput_ = (e.target as HTMLInputElement).value
  }

  private onConfirmAddSiteClick_() {
    // Matched by bare hostname (see
    // GestureController::IsEnabledForCurrentSite()), so a pasted full
    // URL is normalized down to just its hostname.
    const input = this.addSiteInput_.trim()
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
    if (this.editingSiteOrigin_ !== null &&
        this.editingSiteOrigin_ !== host) {
      this.deletePrefDictEntry(
        GESTURES_DISALLOWED_SITES_PREF, this.editingSiteOrigin_)
    }
    this.setPrefDictEntry(GESTURES_DISALLOWED_SITES_PREF, host, true)
    this.showAddSiteDialog_ = false
  }

  private onRemoveSiteClick_(e: DomRepeatEvent<DisallowedSiteRow>) {
    const row = e.model.item
    this.deletePrefDictEntry(GESTURES_DISALLOWED_SITES_PREF, row.origin)
  }

  private onSensitivityChanged_(e: Event) {
    const input = e.target as HTMLInputElement
    const value = Number(input.value)
    // Snap to the default when released close to it, so it's easy to land
    // back on exactly the out-of-the-box sensitivity.
    const snapped =
      Math.abs(value - DEFAULT_SENSITIVITY_IN_PIXELS) <=
          SENSITIVITY_SNAP_THRESHOLD_IN_PIXELS
        ? DEFAULT_SENSITIVITY_IN_PIXELS
        : value
    this.setPrefValue(GESTURES_SENSITIVITY_IN_PIXELS_PREF, snapped)
  }

  private onCaptureCommandChanged_(e: Event) {
    this.selectedCaptureCommand_ =
      Number((e.target as HTMLSelectElement).value)
  }

  private onEditCaptureUrlClick_() {
    this.urlDialogContext_ =
      { isCapture: true, gestureCommand: this.selectedCaptureCommand_ }
    this.urlDialogInput_ = this.selectedCaptureNavigateToUrl_
    this.showUrlDialog_ = true
  }

  private onAddCaptureChainedActionClick_(e: Event) {
    // Rebuilding capturedChainedActions_ below replaces this row's DOM;
    // blur first so focus settles on the body instead of wherever it lands
    // afterward.
    ;(e.target as HTMLElement).blur()
    this.capturedChainedActions_ = [
      ...this.capturedChainedActions_,
      { gestureCommand: GestureCommand.UNASSIGNED, navigateToUrl: '' },
    ]
  }

  private onRemoveCapturePrimaryActionClick_() {
    const [newPrimary, ...rest] = this.capturedChainedActions_
    this.selectedCaptureCommand_ = newPrimary.gestureCommand
    this.selectedCaptureNavigateToUrl_ = newPrimary.navigateToUrl
    this.capturedChainedActions_ = rest
  }

  private onCaptureChainedActionCommandChanged_(
    e: Event & { model: { chainedAction: ChainedActionRow } }) {
    const gestureCommand =
      Number((e.target as HTMLSelectElement).value) as GestureCommand
    this.capturedChainedActions_ = this.capturedChainedActions_.map(
      (action) => action === e.model.chainedAction
        ? { ...action, gestureCommand }
        : action)
  }

  private onEditCaptureChainedUrlClick_(
    e: Event & { model: { chainedAction: ChainedActionRow } }) {
    this.urlDialogContext_ = {
      isCapture: true,
      chainedAction: e.model.chainedAction,
      gestureCommand: e.model.chainedAction.gestureCommand,
    }
    this.urlDialogInput_ = e.model.chainedAction.navigateToUrl
    this.showUrlDialog_ = true
  }

  private onRemoveCaptureChainedActionClick_(
    e: Event & { model: { chainedAction: ChainedActionRow } }) {
    this.capturedChainedActions_ = this.capturedChainedActions_.filter(
      (action) => action !== e.model.chainedAction)
  }

  private onCaptureChainedActionDragStart_(
    e: DragEvent & { model: { chainedAction: ChainedActionRow } }) {
    this.dragContext_ = {
      isCapture: true,
      draggedAction: e.model.chainedAction,
    }
    e.dataTransfer!.effectAllowed = 'move'
    // Some drag implementations only treat the drag as valid, and fire
    // `drop`, once `dataTransfer` actually carries data.
    e.dataTransfer!.setData('text/plain', '')
  }

  private onCaptureChainedActionDrop_(
    e: DragEvent & { model: { chainedAction: ChainedActionRow } }) {
    e.preventDefault()
    const context = this.dragContext_
    this.dragContext_ = null
    if (!context?.isCapture) {
      return
    }
    if (context.draggedAction === null) {
      const swapped = this.swapPrimaryWithChainedAction_(
        { gestureCommand: this.selectedCaptureCommand_,
          navigateToUrl: this.selectedCaptureNavigateToUrl_ },
        this.capturedChainedActions_, e.model.chainedAction)
      this.selectedCaptureCommand_ = swapped.primary.gestureCommand
      this.selectedCaptureNavigateToUrl_ = swapped.primary.navigateToUrl
      this.capturedChainedActions_ = swapped.chainedActions
      return
    }
    this.capturedChainedActions_ = this.reorderChainedActions_(
      this.capturedChainedActions_, context.draggedAction,
      e.model.chainedAction)
  }

  private onCapturePrimaryActionDragStart_(e: DragEvent) {
    this.dragContext_ = { isCapture: true, draggedAction: null }
    e.dataTransfer!.effectAllowed = 'move'
    e.dataTransfer!.setData('text/plain', '')
  }

  private onCapturePrimaryActionDrop_(e: DragEvent) {
    e.preventDefault()
    const context = this.dragContext_
    this.dragContext_ = null
    if (!context?.isCapture || context.draggedAction === null) {
      return
    }
    const swapped = this.swapPrimaryWithChainedAction_(
      { gestureCommand: this.selectedCaptureCommand_,
        navigateToUrl: this.selectedCaptureNavigateToUrl_ },
      this.capturedChainedActions_, context.draggedAction)
    this.selectedCaptureCommand_ = swapped.primary.gestureCommand
    this.selectedCaptureNavigateToUrl_ = swapped.primary.navigateToUrl
    this.capturedChainedActions_ = swapped.chainedActions
  }

  private onCaptureMouseDown_(e: MouseEvent) {
    this.isCapturing_ = true
    this.capturedPoints_ = [this.toCaptureBoxPoint_(e)]
    this.hasCapturedGesture_ = false
    this.updateCapturedPointsString_()
  }

  private onCaptureMouseMove_(e: MouseEvent) {
    if (!this.isCapturing_) {
      return
    }
    this.capturedPoints_.push(this.toCaptureBoxPoint_(e))
    this.updateCapturedPointsString_()
  }

  private onCaptureMouseUp_() {
    this.isCapturing_ = false
    this.hasCapturedGesture_ = this.capturedPoints_.length >= 2
  }

  // Dragging out of the box means the shape was drawn incorrectly; discard the
  // in-progress capture immediately rather than keeping a truncated shape or
  // requiring the confirm dialog used for the explicit Clear button.
  private onCaptureMouseLeave_() {
    if (!this.isCapturing_) {
      return
    }
    this.isCapturing_ = false
    this.capturedPoints_ = []
    this.hasCapturedGesture_ = false
    this.updateCapturedPointsString_()
  }

  private onClearCaptureClick_() {
    this.showClearGestureConfirmDialog_ = true
  }

  private onClearGestureDialogClose_() {
    this.showClearGestureConfirmDialog_ = false
  }

  private onConfirmClearGestureClick_() {
    this.capturedPoints_ = []
    this.hasCapturedGesture_ = false
    this.selectedCaptureCommand_ = GestureCommand.UNASSIGNED
    this.selectedCaptureNavigateToUrl_ = ''
    this.capturedChainedActions_ = []
    this.updateCapturedPointsString_()
    this.showClearGestureConfirmDialog_ = false
  }

  private updateCapturedPointsString_() {
    this.capturedPathData_ = smoothedSvgPath(this.capturedPoints_)
  }

  private toCaptureBoxPoint_(e: MouseEvent): CustomGesturePoint {
    const box = this.shadowRoot!.querySelector('#captureBox') as HTMLElement
    const rect = box.getBoundingClientRect()
    return { x: e.clientX - rect.left, y: e.clientY - rect.top }
  }

  private onSaveCapturedGestureClick_() {
    if (this.capturedPoints_.length < 2) {
      return
    }

    const entry: CustomGestureEntry = {
      id: `${Date.now()}`,
      points: this.capturedPoints_,
      actions: buildActionEntries(
        {
          gestureCommand: this.selectedCaptureCommand_,
          navigateToUrl: this.selectedCaptureNavigateToUrl_,
        },
        this.capturedChainedActions_),
    }

    this.trySaveCustomGesture_(entry)
  }

  // Saves `entry` immediately. A shape clash with another custom or
  // directional gesture (see `shapeClashDescription_`/
  // `anyCustomGestureMatchesDirectionalKey_`) doesn't block the save; the
  // saved list highlights it afterward instead of gating the save itself
  // behind a confirmation dialog. Both gestures are allowed to share the
  // same action; a command already used elsewhere is flagged the same way.
  private trySaveCustomGesture_(entry: CustomGestureEntry) {
    this.commitSavedCustomGesture_(entry)
  }

  private commitSavedCustomGesture_(entry: CustomGestureEntry) {
    const existingCustomGestures = this.existingCustomGestures_()
    // Write the whole array back via setPrefValue rather than
    // appendPrefListItem: the latter uses Polymer's push(), which only
    // fires array-splice notifications, not the plain
    // prefs.<path>.value-changed notification our complex observers
    // (onCustomGesturesPrefChanged_) depend on.
    this.setPrefValue(
      GESTURES_CUSTOM_GESTURES_PREF, [...existingCustomGestures, entry])
    this.capturedPoints_ = []
    this.hasCapturedGesture_ = false
    this.selectedCaptureCommand_ = GestureCommand.UNASSIGNED
    this.selectedCaptureNavigateToUrl_ = ''
    this.capturedChainedActions_ = []
    this.updateCapturedPointsString_()
  }

  private onRemoveCustomGestureClick_(e: DomRepeatEvent<CustomGestureRow>) {
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
