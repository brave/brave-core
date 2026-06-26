// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { createRoot } from 'react-dom/client'
import * as React from 'react'
import { StyleSheetManager } from 'styled-components'
import styled from 'styled-components'
import { setIconBasePath } from '@brave/leo/react/icon'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import { color, font, spacing, radius, typography } from '@brave/leo/tokens/css/variables'
import {
    EmailAliasesServiceObserverInterface,
    EmailAliasesServiceObserverReceiver,
    EmailAliasesPromoHandlerInterface,
    EmailAliasesPromoHandler,
} from 'gen/brave/components/email_aliases/email_aliases.mojom.m'
import Col from './content/styles/Col'
import Row from './content/styles/Row'
import NytIcon from './content/assets/nyt-logo.svg'

// ---------------------------------------------------------------------------
// Shared modal primitives — mirrors email_aliases_modal.tsx naming
// ---------------------------------------------------------------------------

const ModalCol = styled(Col)`
  row-gap: ${spacing['2Xl']};
`

const ModalHeader = styled(Row)`
  width: 100%;
  justify-content: space-between;
  align-items: center;
  box-sizing: border-box;
  flex-shrink: 0;
`

const ModalTitle = styled.h4`
  color: ${color.text.secondary};
  font: ${font.heading.h4};
  font-weight: 600;
  line-height: ${typography.heading.h4.lineHeight};
  margin: 0;
`
export const RightAlignedItem = styled.div`
  margin-left: auto;
  text-align: right;
`

const ModalDescription = styled.p`
  margin: 0;
  color: ${color.text.primary};
  font: ${font.default.regular};
  size: ${typography.default.regular.fontSize};
  line-height: ${typography.default.regular.lineHeight};
  letter-spacing: ${typography.letterSpacing.default};
  list-spacing: ${typography.paragraphSpacing.default};
  weight: ${typography.default.regular.fontWeight};
  paragraph-spacing: ${typography.paragraphSpacing.default};
`

const ModalSectionCol = styled(Col)`
    row-gap: ${spacing['2Xl']};
`

const ModalLabel = styled.span`
  font: ${font.small.regular};
  color: ${color.text.tertiary};
`

const ModalFooter = styled(Col)`
  padding: ${spacing.m};
  flex-shrink: 0;
  gap: ${spacing.s};
`

// ---------------------------------------------------------------------------
// Demo cards
// ---------------------------------------------------------------------------

const DemoArea = styled(Col)`
  align-items: center;
  gap: ${spacing.s};
`

const CardsRow = styled(Row)`
  align-items: center;
  justify-content: center;
  width: 100%;
  position: relative;
`

const SiteCard = styled(Col) <{ $faded?: boolean }>`
  flex: 1;
  border-radius: ${radius.m};
  border: 1px solid ${color.divider.subtle};
  padding: ${spacing.s};
  gap: 6px;
  background: ${color.container.background};
  opacity: ${({ $faded }) => ($faded ? 0.45 : 1)};
  min-width: 0;
  overflow: hidden;
`

const SiteCardFocused = styled(SiteCard)`
  flex: 1.4;
  opacity: 1;
  gap: ${spacing.l};
  z-index: 1;
  position: relative;
  /* Pull the side cards underneath the focused card's edges. */
  margin: 0 calc(-1 * ${spacing['2Xl']});
  /* Extra vertical padding makes the focused card taller than its
     neighbors so it stands out in front. */
  padding: ${spacing['2Xl']} ${spacing['2Xl']};
  box-shadow: 0 8px 24px rgba(0, 0, 0, 0.16);
`

const SiteTitle = styled.span`
  font: ${font.default.semibold};
  font-size: 13px;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
`

// const NytTitle = styled.div`
//   font-family: 'Times New Roman', Times, serif;
//   font-size: 15px;
//   font-weight: bold;
//   text-align: center;
//   color: ${color.text.primary};
//   white-space: nowrap;
//   margin-bottom: ${spacing.xl};
//`

const AliasChip = styled.div`
  border-radius: ${radius.s};
  border: 1px solid ${color.divider.subtle};
  padding: 4px 6px;
  font-size: 12px;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
  background: ${color.container.highlight};
`

const AliasChipFocused = styled(AliasChip)`
  padding: 6px 10px;
  font-size: 13px;
  text-align: center;
  background: ${color.container.background};
`

const AliasPart = styled.span`
  color: #7c3aed;
`

const AliasNumber = styled.span`
  color: #ea580c;
`

// ---------------------------------------------------------------------------
// Arrow
// ---------------------------------------------------------------------------

const ArrowWrapper = styled(Col)`
  align-items: center;
  color: #ea580c;
  line-height: 1;
`

const ArrowLine = styled.div`
  width: 2px;
  height: 18px;
  background: #ea580c;
`

const ArrowHead = styled.div`
  width: 0;
  height: 0;
  border-left: 5px solid transparent;
  border-right: 5px solid transparent;
  border-top: 7px solid #ea580c;
`

// ---------------------------------------------------------------------------
// Real-email row
// ---------------------------------------------------------------------------

const RealEmailRow = styled(Row)`
  align-items: center;
  gap: ${spacing.s};
  border-radius: ${radius.m};
  border: 1px solid ${color.divider.subtle};
  padding: 8px 14px;
  background: ${color.container.background};
  box-sizing: border-box;
  color: ${color.text.secondary};
  font: ${font.default.regular};
`

// ---------------------------------------------------------------------------
// Component
// ---------------------------------------------------------------------------

const EmailAliasesPromo = ({
    onClose,
    onGetStarted,
}: {
    onClose: () => void
    onGetStarted: () => void
}) => {
    return (
        <ModalCol>
            <ModalHeader>
                <ModalTitle>Email aliases</ModalTitle>
                <RightAlignedItem>
                    <Button fab kind='plain-faint' onClick={onClose}>
                        <Icon name='close' />
                    </Button>
                </RightAlignedItem>
            </ModalHeader>

            <ModalSectionCol>
                <ModalDescription>
                    Create unique, random addresses that forward to your Brave account
                    email and can be deleted at any time. Keep your actual email address
                    from being disclosed or used by advertisers.
                </ModalDescription>

                <DemoArea>
                    <CardsRow>
                        {/* Left — faded Walmart card */}
                        <SiteCard $faded>
                            <SiteTitle style={{ color: '#0071ce' }}>Walmart ✦</SiteTitle>
                            <ModalLabel>Register on Wal…</ModalLabel>
                            <AliasChip>
                                <AliasPart>lion.paw.</AliasPart>
                                <AliasNumber>75</AliasNumber>
                                <span>…</span>
                            </AliasChip>
                        </SiteCard>

                        {/* Center — focused NYT card */}
                        <SiteCardFocused>
                            {/* <NytTitle>The New York Times</NytTitle> */}
                            <img src={NytIcon} alt="The New York Times" />
                            <ModalLabel>Enter your email to continue</ModalLabel>
                            <AliasChipFocused>
                                <AliasPart>cat.mane.</AliasPart>
                                <AliasNumber>2947</AliasNumber>
                                <span>@bravealias.com</span>
                            </AliasChipFocused>
                        </SiteCardFocused>

                        {/* Right — faded generic store card */}
                        <SiteCard $faded>
                            <SiteTitle style={{ color: color.text.secondary }}>
                                store.com
                            </SiteTitle>
                            <ModalLabel>&nbsp;</ModalLabel>
                            <AliasChip>
                                <span>…ealias.com</span>
                            </AliasChip>
                        </SiteCard>
                    </CardsRow>

                    <ArrowWrapper>
                        <ArrowLine />
                        <ArrowHead />
                    </ArrowWrapper>

                    <RealEmailRow>
                        <Icon name='email-shield' />
                        youremail@example.com
                    </RealEmailRow>
                </DemoArea>
                <ModalDescription>
                    To use Email aliases, you need to be logged in to your Brave account.
                    Get started by clicking the button below.
                </ModalDescription>
            </ModalSectionCol>

            <ModalFooter>
                <Button kind='filled' onClick={onGetStarted} style={ 'align-self: flex-end' }>
                    Get started
                </Button>
            </ModalFooter>
        </ModalCol>
    )
}

// ---------------------------------------------------------------------------
// Connected wrapper
// ---------------------------------------------------------------------------

export const EmailAliasesPromoConnected = ({
    emailAliasesPromoHandler,
    bindObserver,
}: {
    emailAliasesPromoHandler: EmailAliasesPromoHandlerInterface
    bindObserver: (observer: EmailAliasesServiceObserverInterface) => () => void
}) => {
    return (
        <EmailAliasesPromo
            onClose={() => emailAliasesPromoHandler.onPromoClosed()}
            onGetStarted={() => emailAliasesPromoHandler.onPromoClosed()}
        />
    )
}

// ---------------------------------------------------------------------------
// Mount
// ---------------------------------------------------------------------------

const mount = () => {
    const rootElement = document.getElementById('mountPoint')!
    const emailAliasesPromoHandler = EmailAliasesPromoHandler.getRemote()
    const bindObserver = (observer: EmailAliasesServiceObserverInterface) => {
        const observerReceiver = new EmailAliasesServiceObserverReceiver(observer)
        return () => observerReceiver.$.close()
    }
    setIconBasePath('//resources/brave-icons')
    createRoot(rootElement).render(
        <StyleSheetManager>
            <EmailAliasesPromoConnected
                emailAliasesPromoHandler={emailAliasesPromoHandler}
                bindObserver={bindObserver}
            />
        </StyleSheetManager>,
    )
}

document.addEventListener('DOMContentLoaded', mount)