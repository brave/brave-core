/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import ButtonMenu from '@brave/leo/react/buttonMenu'
import Icon from '@brave/leo/react/icon'
import Label from '@brave/leo/react/label'
import classnames from '$web-common/classnames'
import { getLocale } from '$web-common/locale'
import * as Mojom from '../../../common/mojom'
import {
  AUTOMATIC_MODEL_KEY,
  getModelIcon,
  NEAR_AI_LEARN_MORE_URL,
} from '../../../common/constants'
import { formatModelCapabilitiesSubtitle } from '../../model_utils'
import { NearIcon } from '../near_label/near_label'
import styles from './model_menu_item_style.module.scss'

const LONG_PRESS_MS = 500

interface ModelContentProps {
  model: Mojom.Model
  isCurrent: boolean
  showDetails?: boolean
  showPremiumLabel?: boolean
  isDisabled?: boolean
  isPinned?: boolean
  showCapabilitySubtitle?: boolean
  onClickLearnMore?: () => void
  trailingContent?: React.ReactNode
}

const ModelContent = (props: ModelContentProps) => {
  const {
    model,
    isCurrent,
    showDetails,
    showPremiumLabel,
    isDisabled,
    showCapabilitySubtitle,
    onClickLearnMore,
    trailingContent,
  } = props

  const isCustomModel = model.options.customModelOptions
  const isOllamaModel = !!(
    model.options.customModelOptions?.endpoint.url === Mojom.OLLAMA_ENDPOINT
  )

  const label = React.useMemo(() => {
    if (isCurrent) {
      return (
        <Label
          mode='loud'
          color='primary'
          className={styles.modelLabel}
        >
          {getLocale(S.CHAT_UI_CURRENT_LABEL)}
        </Label>
      )
    }
    if (model.isNearModel) {
      return (
        <Label
          mode='outline'
          color='neutral'
          className={styles.modelLabel}
        >
          {getLocale(S.CHAT_UI_MODEL_BETA_LABEL)}
        </Label>
      )
    }
    if (
      model.options.leoModelOptions?.access === Mojom.ModelAccess.PREMIUM
      && showPremiumLabel
    ) {
      return (
        <Label
          mode='outline'
          color='blue'
          className={styles.modelLabel}
        >
          {getLocale(S.CHAT_UI_MODEL_PREMIUM_LABEL_NON_PREMIUM)}
        </Label>
      )
    }
    if (isCustomModel) {
      return (
        <Label
          mode='default'
          color='blue'
          className={styles.modelLabel}
        >
          {getLocale(
            isOllamaModel
              ? S.CHAT_UI_MODEL_OLLAMA_LABEL
              : S.CHAT_UI_MODEL_LOCAL_LABEL,
          )}
        </Label>
      )
    }
    return null
  }, [isCurrent, showPremiumLabel, isCustomModel, isOllamaModel, model])

  const subtitle = React.useMemo(() => {
    if (isCustomModel) {
      return model.options.customModelOptions?.modelRequestName ?? ''
    }

    if (showCapabilitySubtitle) {
      // Auto and Near models keep their descriptive localized subtitle.
      if (
        model.key === AUTOMATIC_MODEL_KEY
        || model.isNearModel
        || !(model.capabilities?.length)
      ) {
        return getLocale(
          `CHAT_UI_${model.key.toUpperCase().replaceAll('-', '_')}_SUBTITLE`,
        )
      }
      return formatModelCapabilitiesSubtitle(model.capabilities)
    }

    return getLocale(
      `CHAT_UI_${model.key.toUpperCase().replaceAll('-', '_')}_SUBTITLE`,
    )
  }, [isCustomModel, model, showCapabilitySubtitle])

  return (
    <>
      <div
        className={classnames({
          [styles.modelIcon]: true,
          [styles.modelIconDetailed]: showDetails,
          [styles.disabled]: isDisabled,
        })}
        data-key={model.key}
      >
        <Icon name={getModelIcon(model)} />
      </div>
      <div className={styles.column}>
        <div className={styles.nameAndLabel}>
          <div
            className={classnames({
              [styles.modelName]: true,
              [styles.disabled]: isDisabled,
            })}
          >
            <span className={styles.modelNameText}>{model.displayName}</span>
            {model.isNearModel && <NearIcon />}
          </div>
          <div className={styles.labelAndActions}>{label}</div>
        </div>
        {showDetails && (
          <>
            <p className={styles.modelSubtitle}>{subtitle}</p>
            {onClickLearnMore && model.isNearModel && (
              <a
                // While we preventDefault, we still need to pass the href
                // here so we can continue to show link previews.
                href={NEAR_AI_LEARN_MORE_URL}
                className={styles.learnMoreLink}
                onClick={(e) => {
                  e.preventDefault()
                  e.stopPropagation()
                  onClickLearnMore()
                }}
              >
                {getLocale(S.CHAT_UI_LEARN_MORE)}
              </a>
            )}
          </>
        )}
      </div>
      {/* Sibling of icon/column so the overlay can span the full item height. */}
      {trailingContent}
    </>
  )
}

interface ModelOptionsMenuProps {
  modelKey: string
  isPinned?: boolean
  isDefault?: boolean
  isOpen: boolean
  canPin: boolean
  canSetDefault: boolean
  onOpenChange: (open: boolean) => void
  onTogglePin?: () => void
  onSetAsDefault?: () => void
}

function ModelOptionsMenu(props: ModelOptionsMenuProps) {
  const {
    modelKey,
    isPinned,
    isDefault,
    isOpen,
    canPin,
    canSetDefault,
    onOpenChange,
    onTogglePin,
    onSetAsDefault,
  } = props

  const wrapRef = React.useRef<HTMLDivElement>(null)

  // Closing returns focus to the more button; that leaves the parent
  // leo-menu-item matching :focus-within, which keeps our hover styles on.
  const setOpen = React.useCallback(
    (open: boolean) => {
      onOpenChange(open)
      if (open) {
        return
      }
      queueMicrotask(() => {
        const wrap = wrapRef.current
        const active = document.activeElement
        if (
          wrap
          && active instanceof HTMLElement
          && (wrap === active || wrap.contains(active))
        ) {
          active.blur()
        }
      })
    },
    [onOpenChange],
  )

  return (
    <div
      ref={wrapRef}
      className={classnames({
        [styles.optionsWrap]: true,
        [styles.optionsWrapOpen]: isOpen,
      })}
      onClick={(e) => {
        e.preventDefault()
        e.stopPropagation()
      }}
      onMouseDown={(e) => e.stopPropagation()}
    >
      <ButtonMenu
        className={styles.optionsMenu}
        isOpen={isOpen}
        onChange={(e) => setOpen(e.isOpen)}
        positionStrategy='fixed'
        placement='bottom-end'
      >
        <Button
          slot='anchor-content'
          fab
          kind='plain-faint'
          size='tiny'
          className={styles.moreButton}
          aria-label={getLocale(S.CHAT_UI_MODEL_MORE_OPTIONS)}
          data-testid={`model-options-${modelKey}`}
          onClick={(e) => {
            // Stop bubbling so the parent model row isn't selected, and
            // toggle here because stopPropagation also blocks Leo's anchor
            // click handler on this nested ButtonMenu.
            e.preventDefault()
            e.stopPropagation()
            setOpen(!isOpen)
          }}
        >
          <Icon name='more-vertical' />
        </Button>
        {canPin && (
          <leo-menu-item
            class={styles.leoDropdownItem}
            data-testid={`pin-${modelKey}`}
            onClick={(e) => {
              e.stopPropagation()
              onTogglePin?.()
              setOpen(false)
            }}
          >
            <Icon name={isPinned ? 'pin-disable' : 'pin'} />
            <span className={styles.leoDropdownItemLabel}>
              {getLocale(
                isPinned ? S.CHAT_UI_UNPIN_LABEL : S.CHAT_UI_PIN_LABEL,
              )}
            </span>
          </leo-menu-item>
        )}
        {canSetDefault && (
          <leo-menu-item
            class={classnames({
              [styles.leoDropdownItem]: true,
              [styles.leoDropdownItemSelected]: isDefault,
            })}
            data-testid={`set-default-${modelKey}`}
            aria-selected={isDefault ? 'true' : null}
            onClick={(e) => {
              e.stopPropagation()
              if (isDefault) {
                setOpen(false)
                return
              }
              onSetAsDefault?.()
              setOpen(false)
            }}
          >
            <Icon
              name={isDefault ? 'check-circle-filled' : 'check-circle-outline'}
            />
            <span className={styles.leoDropdownItemLabel}>
              {getLocale(S.CHAT_UI_SET_AS_DEFAULT_MODEL)}
            </span>
          </leo-menu-item>
        )}
      </ButtonMenu>
    </div>
  )
}

interface MenuItemProps extends ModelContentProps {
  isDefault?: boolean
  onClick: () => void
  isMobile?: boolean
  isOptionsOpen?: boolean
  onOptionsOpenChange?: (open: boolean) => void
  onTogglePin?: () => void
  onSetAsDefault?: () => void
}

export function ModelMenuItem(props: MenuItemProps) {
  const {
    model,
    isCurrent,
    showDetails,
    showPremiumLabel,
    isDisabled,
    isPinned,
    isDefault,
    showCapabilitySubtitle,
    isMobile,
    isOptionsOpen = false,
    onOptionsOpenChange,
    onClick,
    onClickLearnMore,
    onTogglePin,
    onSetAsDefault,
  } = props

  const longPressTimer = React.useRef<number | undefined>(undefined)
  const didLongPress = React.useRef(false)

  // Automatic cannot be pinned; other models with a pin handler can.
  const canPin = !!onTogglePin && model.key !== AUTOMATIC_MODEL_KEY
  const canSetDefault = !!onSetAsDefault
  const showOptions = canPin || canSetDefault

  const setOptionsOpen = React.useCallback(
    (open: boolean) => {
      onOptionsOpenChange?.(open)
    },
    [onOptionsOpenChange],
  )

  const clearLongPressTimer = React.useCallback(() => {
    if (longPressTimer.current !== undefined) {
      window.clearTimeout(longPressTimer.current)
      longPressTimer.current = undefined
    }
  }, [])

  React.useEffect(() => clearLongPressTimer, [clearLongPressTimer])

  // Touch/context handlers live on an inner wrapper — Leo's leo-menu-item
  // typings only expose onClick, not React touch event props.
  const longPressHandlers = isMobile && showOptions
    ? {
        onTouchStart: () => {
          didLongPress.current = false
          clearLongPressTimer()
          longPressTimer.current = window.setTimeout(() => {
            didLongPress.current = true
            setOptionsOpen(true)
          }, LONG_PRESS_MS)
        },
        onTouchEnd: clearLongPressTimer,
        onTouchMove: clearLongPressTimer,
        onTouchCancel: clearLongPressTimer,
        onContextMenu: (e: React.MouseEvent) => {
          e.preventDefault()
          e.stopPropagation()
          clearLongPressTimer()
          didLongPress.current = true
          setOptionsOpen(true)
        },
      }
    : undefined

  return (
    <leo-menu-item
      class={isOptionsOpen ? styles.menuItemOptionsOpen : undefined}
      data-key={model.key}
      data-testid={model.key}
      aria-selected={isCurrent ? 'true' : null}
      onClick={(e) => {
        if (didLongPress.current) {
          e.preventDefault()
          e.stopPropagation()
          didLongPress.current = false
          return
        }
        onClick()
      }}
    >
      <div
        className={styles.menuItemHitArea}
        {...longPressHandlers}
      >
        <ModelContent
          model={model}
          isCurrent={isCurrent}
          showPremiumLabel={showPremiumLabel}
          showDetails={showDetails}
          isDisabled={isDisabled}
          isPinned={isPinned}
          showCapabilitySubtitle={showCapabilitySubtitle}
          onClickLearnMore={onClickLearnMore}
          trailingContent={
            showOptions ? (
              <ModelOptionsMenu
                modelKey={model.key}
                isPinned={isPinned}
                isDefault={isDefault}
                isOpen={isOptionsOpen}
                canPin={canPin}
                canSetDefault={canSetDefault}
                onOpenChange={setOptionsOpen}
                onTogglePin={onTogglePin}
                onSetAsDefault={onSetAsDefault}
              />
            ) : undefined
          }
        />
      </div>
    </leo-menu-item>
  )
}

export function ModelOption(props: ModelContentProps) {
  const {
    model,
    isCurrent,
    showDetails,
    showPremiumLabel,
    isDisabled,
    isPinned,
    showCapabilitySubtitle,
  } = props

  const content = (
    <ModelContent
      model={model}
      isCurrent={isCurrent}
      showPremiumLabel={showPremiumLabel}
      showDetails={showDetails}
      isDisabled={isDisabled}
      isPinned={isPinned}
      showCapabilitySubtitle={showCapabilitySubtitle}
    />
  )

  // There is currently no way to disable a menu item in nala dropdowns,
  // so we need to return as a div instead of a leo-option.
  if (isDisabled) {
    return <div className={styles.disabledOption}>{content}</div>
  }

  return <leo-option value={model.key}>{content}</leo-option>
}
