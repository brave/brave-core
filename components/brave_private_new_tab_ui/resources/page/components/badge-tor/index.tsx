// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import styled, { css } from 'styled-components'
import Icon from '@brave/leo/react/icon'
import ProgressRing from '@brave/leo/react/progressRing'
import * as leo from '@brave/leo/tokens/css/variables'
import { getLocale, formatLocale } from '$web-common/locale'
import getPageHandlerInstance, { ConnectionStatus } from '../../api/brave_page_handler'

type PillKind = 'disconnected' | 'progress' | 'connected'

const Pill = styled.div<{ kind: PillKind }>`
  display: inline-flex;
  align-items: center;
  gap: ${leo.spacing.l};
  position: relative;
  overflow: hidden;
  box-sizing: border-box;
  padding: ${leo.spacing.l} ${leo.spacing.xl};
  border-radius: ${leo.radius.full};
  box-shadow: ${leo.effect.elevation['01']};
  color: ${leo.color.white};
  --leo-icon-size: var(--leo-icon-l);
  --leo-icon-color: ${leo.color.white};
  --leo-progressring-size: var(--leo-icon-l);
  --leo-progressring-color: ${leo.color.white};

  ${p => p.kind === 'disconnected' && css`
    background: ${leo.color.primitive.red[50]};
  `}

  ${p => p.kind === 'connected' && css`
    background: ${leo.color.primitive.green[60]};
  `}

  ${p => p.kind === 'progress' && css`
    background: var(--window-background);
  `}

  width: max-content;
  max-width: calc(100vw - 2 * ${leo.spacing.xl});
  transition: width ${leo.duration.m} ${leo.easing.inOut};

  @media (prefers-reduced-motion: reduce) {
    transition: none;
  }
`

const ProgressFill = styled.div`
  position: absolute;
  inset: 0 auto 0 0;
  background: var(--window-surface-strong);
  transition: width ${leo.duration.m} ${leo.easing.inOut};
`

const IconBox = styled.span`
  display: flex;
  flex-shrink: 0;
  position: relative;
`

const Text = styled.div`
  display: flex;
  flex-direction: column;
  min-width: 0;
  position: relative;
`

const Title = styled.span`
  display: block;
  overflow: hidden;
  font: ${leo.font.default.semibold};
  font-variant-numeric: tabular-nums;
  letter-spacing: ${leo.typography.default.semibold.letterSpacing};
  white-space: nowrap;
  text-overflow: ellipsis;
`

const Subtitle = styled.span`
  display: block;
  overflow: hidden;
  font: ${leo.font.small.regular};
  letter-spacing: ${leo.typography.small.regular.letterSpacing};
  color: var(--window-text-primary);
  white-space: nowrap;
  text-overflow: ellipsis;
`

const Help = styled.p`
  max-width: 300px;
  margin: ${leo.spacing.m} 0 0;
  font: ${leo.font.small.regular};
  color: var(--window-text-primary);

  a {
    color: inherit;
    cursor: pointer;
    text-decoration: underline;
  }
`

interface Props {
  isConnected: boolean
  progress: string
  message: string
  connectionStatus: ConnectionStatus
}

function splitMessage (key: string) {
  return getLocale(key).split(/\$\d+/g)
}

function contactBraveSupport () {
  getPageHandlerInstance().pageHandler.goToBraveSupport()
}

function StuckHelp () {
  const [open, settings, reenable] = splitMessage('torHelpDisconnectedReenable')
  const [
    before,
    havingTrouble,
    tryUsing,
    bridges,
    or,
    contactSupport,
    rest
  ] = splitMessage('torHelpDisconnectedBridges')

  return (
    <Help>
      {open}<a href="chrome://settings/privacy">{settings}</a>{reenable}
      <br /><br />
      {before}<strong>{havingTrouble}</strong>
      {tryUsing}<a href="chrome://settings/privacy">{bridges}</a>
      {or}
      <a onClick={contactBraveSupport}>{contactSupport}</a>
      {rest}
    </Help>
  )
}

function parseProgress (progress: string) {
  const value = Number.parseFloat(progress)
  return Number.isFinite(value) ? Math.min(Math.max(value, 0), 100) : 0
}

// `width: auto` does not transition when the label changes length, so the
// pill is measured and its used width is animated explicitly.
function useAnimatedPillWidth (
  pillRef: React.RefObject<HTMLDivElement | null>,
  signature: string
) {
  React.useLayoutEffect(() => {
    const el = pillRef.current
    if (!el) return

    const reduced = window.matchMedia('(prefers-reduced-motion: reduce)').matches
    const current = el.getBoundingClientRect().width
    el.style.transition = 'none'
    el.style.width = 'max-content'
    const next = el.getBoundingClientRect().width

    if (reduced || Math.abs(current - next) < 0.5) {
      el.style.width = `${next}px`
      el.style.transition = ''
      return
    }

    el.style.width = `${current}px`
    // Commit the start width before the target, or the transition is skipped.
    el.getBoundingClientRect()
    el.style.transition = ''
    el.style.width = `${next}px`
  }, [pillRef, signature])
}

function BadgeTor (props: Props) {
  const isConnected =
    props.isConnected || props.connectionStatus === ConnectionStatus.kConnected
  const isStuck = props.connectionStatus === ConnectionStatus.kConnectionStuck
  const isSlow = props.connectionStatus === ConnectionStatus.kConnectionSlow
  const isConnecting =
    !isConnected && !isStuck &&
    (props.connectionStatus === ConnectionStatus.kConnecting || isSlow)

  let title = getLocale('torStatusDisconnected')
  let subtitle = ''
  if (isConnecting) {
    title = isSlow
      ? getLocale('torStatusConnectionSlow')
      : formatLocale('torStatusInitializing', { $1: props.progress })
    subtitle = isSlow
      ? getLocale('torStatusConnectionSlowDesc')
      : props.message
  } else if (isConnected) {
    title = getLocale('torStatusConnected')
  } else if (isStuck) {
    title = getLocale('torStatusConnectionFailed')
  }

  const kind: PillKind = isConnecting
    ? 'progress'
    : isConnected
      ? 'connected'
      : 'disconnected'

  const pillRef = React.useRef<HTMLDivElement>(null)
  // Digit count, not the raw percent: tabular figures keep 80 and 81 the same
  // width, and remeasuring on every tick would restart the width animation.
  const progressDigits = isConnecting && !isSlow
    ? String(Math.round(parseProgress(props.progress))).length
    : 0
  useAnimatedPillWidth(
    pillRef,
    `${kind}|${isStuck}|${isSlow}|${progressDigits}|${props.message}`
  )

  return (
    <div>
      <Pill ref={pillRef} kind={kind} role='status' data-testid='tor-status'>
        {isConnecting && (
          <ProgressFill style={{ width: `${parseProgress(props.progress)}%` }} />
        )}
        <IconBox>
          {isConnecting
            ? <ProgressRing />
            : <Icon name='product-tor' />}
        </IconBox>
        <Text>
          <Title>{title}</Title>
          {subtitle && <Subtitle>{subtitle}</Subtitle>}
        </Text>
      </Pill>
      {isStuck && <StuckHelp />}
    </div>
  )
}

export default BadgeTor
