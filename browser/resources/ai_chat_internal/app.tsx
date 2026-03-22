// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import 'chrome://resources/brave/leo.bundle.js'
import * as React from 'react'
import { createRoot } from 'react-dom/client'
import { Service } from 'gen/brave/components/ai_chat/core/common/mojom/ai_chat.mojom.m.js'
import { CharacterType } from 'gen/brave/components/ai_chat/core/common/mojom/common.mojom.m.js'
import ProgressRing from '@brave/leo/react/progressRing'

function formatTimestamp(mojoTime: any): string {
    if (!mojoTime) return 'Unknown'
    try {
        // Mojo TimeTicks uses Windows epoch (1601-01-01)
        const windowsToUnixEpochUs = BigInt(11644473600) * BigInt(1000000)
        const unixMicros = BigInt(mojoTime.internalValue) - windowsToUnixEpochUs
        const unixMs = Number(unixMicros / BigInt(1000))
        return new Date(unixMs).toLocaleString()
    } catch {
        return String(mojoTime)
    }
}

function rawStringify(obj: any): string {
    return JSON.stringify(
        obj,
        (_key, value) =>
            typeof value === 'bigint' ? value.toString() + 'n' : value,
        2
    )
}

function ConversationNavItem({
    conv,
    selected,
    onClick,
}: {
    conv: any
    selected: boolean
    onClick: () => void
}) {
    const [hovered, setHovered] = React.useState(false)
    return (
        <div
            onClick={onClick}
            onMouseEnter={() => setHovered(true)}
            onMouseLeave={() => setHovered(false)}
            style={{
                width: '100%',
                boxSizing: 'border-box',
                padding: 'var(--leo-spacing-m) var(--leo-spacing-l)',
                cursor: 'pointer',
                background: selected
                    ? 'var(--leo-color-container-highlight)'
                    : hovered
                        ? 'var(--leo-color-container-background)'
                        : 'transparent',

                borderLeft: selected
                    ? '3px solid var(--leo-color-text-interactive)'
                    : '3px solid transparent',
                transition: 'background 0.1s',
            }}
        >
            <div style={{
                font: 'var(--leo-font-default-regular)',
                color: 'var(--leo-color-text-primary)',
                overflow: 'hidden',
                textOverflow: 'ellipsis',
                whiteSpace: 'nowrap',
            }}>
                {conv.title || 'Untitled'}
            </div>
            <div style={{
                font: 'var(--leo-font-small-regular)',
                fontFamily: 'monospace',
                color: 'var(--leo-color-text-tertiary)',
                marginTop: 'var(--leo-spacing-xs)',
                overflow: 'hidden',
                textOverflow: 'ellipsis',
                whiteSpace: 'nowrap',
            }}>
                {conv.uuid}
            </div>
        </div>
    )
}

function SidebarToggle({
    open,
    onClick,
}: {
    open: boolean
    onClick: () => void
}) {
    const [hovered, setHovered] = React.useState(false)
    return (
        <button
            onClick={onClick}
            onMouseEnter={() => setHovered(true)}
            onMouseLeave={() => setHovered(false)}
            style={{
                background: 'none',
                border: 'none',
                cursor: 'pointer',
                color: hovered
                    ? 'var(--leo-color-text-primary)'
                    : 'var(--leo-color-text-secondary)',
                font: 'var(--leo-font-default-regular)',
                width: 'var(--leo-icon-m)',
                height: 'var(--leo-icon-m)',
                display: 'flex',
                alignItems: 'center',
                justifyContent: 'center',
                padding: 0,
                flexShrink: 0,
                transition: 'color 0.1s',
            }}
            title={open ? 'Collapse sidebar' : 'Expand sidebar'}
        >
            ☰
        </button>
    )
}

function RawToggleButton({
    show,
    onClick,
}: {
    show: boolean
    onClick: () => void
}) {
    return (
        <button
            onClick={onClick}
            style={{
                background: 'none',
                border: 'none',
                cursor: 'pointer',
                color: 'var(--leo-color-text-interactive)',
                font: 'var(--leo-font-small-regular)',
                padding: 'var(--leo-spacing-xs) 0',
                whiteSpace: 'nowrap',
            }}
        >
            {show ? 'Hide Raw' : 'View Raw'}
        </button>
    )
}

function RawBlock({ data }: { data: any }) {
    return (
        <pre style={{
            marginTop: 'var(--leo-spacing-m)',
            background: 'var(--leo-color-container-background)',
            padding: 'var(--leo-spacing-m)',
            font: 'var(--leo-font-small-regular)',
            fontFamily: 'monospace',
            overflowX: 'auto',
            borderRadius: 'var(--leo-radius-s)',
            border: '1px solid var(--leo-color-divider-subtle)',
            whiteSpace: 'pre-wrap',
            wordBreak: 'break-word',
        }}>
            {rawStringify(data)}
        </pre>
    )
}

function TurnView({ turn }: { turn: any }) {
    const [showRaw, setShowRaw] = React.useState(false)
    const isHuman = turn.characterType === CharacterType.HUMAN

    return (
        <div style={{
            padding: 'var(--leo-spacing-l)',
            borderBottom: '1px solid var(--leo-color-divider-subtle)',
        }}>
            <div style={{
                display: 'flex',
                justifyContent: 'space-between',
                alignItems: 'center',
                marginBottom: 'var(--leo-spacing-s)',
            }}>
                <div style={{
                    display: 'flex',
                    alignItems: 'baseline',
                    gap: 'var(--leo-spacing-m)',
                }}>
                    <span style={{
                        font: 'var(--leo-font-default-semibold)',
                        color: isHuman
                            ? 'var(--leo-color-text-primary)'
                            : 'var(--leo-color-icon-default)',
                    }}>
                        {isHuman ? 'User' : 'Assistant'}
                    </span>
                    <span style={{
                        font: 'var(--leo-font-small-regular)',
                        color: 'var(--leo-color-text-tertiary)',
                    }}>
                        {formatTimestamp(turn.createdTime)}
                    </span>
                </div>
                <RawToggleButton show={showRaw} onClick={() => setShowRaw(!showRaw)} />
            </div>
            <div style={{
                whiteSpace: 'pre-wrap',
                lineHeight: 1.6,
                color: 'var(--leo-color-text-primary)',
            }}>
                {turn.text || ''}
            </div>
            {showRaw && <RawBlock data={turn} />}
        </div>
    )
}

function ConversationDetail({ uuid }: { uuid: string }) {
    const [conversation, setConversation] = React.useState<any>(null)
    const [turns, setTurns] = React.useState<any[]>([])
    const [loading, setLoading] = React.useState(true)
    const [error, setError] = React.useState<string | null>(null)
    const [showConvRaw, setShowConvRaw] = React.useState(false)

    React.useEffect(() => {
        let cancelled = false
        setLoading(true)
        setError(null)
        setShowConvRaw(false)

        async function load() {
            try {
                const handler = Service.getRemote()
                const response = await handler.getConversationData(uuid)
                if (cancelled) return
                setConversation(response.conversation)
                setTurns(response.entries || [])
            } catch (e) {
                if (cancelled) return
                setError(`Error fetching data: ${e}`)
            } finally {
                if (!cancelled) setLoading(false)
            }
        }
        load()
        return () => { cancelled = true }
    }, [uuid])

    if (loading) {
        return (
            <div style={{
                display: 'flex',
                justifyContent: 'center',
                alignItems: 'center',
                height: '100%',
            }}>
                <ProgressRing />
            </div>
        )
    }

    if (error) {
        return (
            <div style={{
                color: 'var(--leo-color-systemfeedback-error-text)',
                padding: 'var(--leo-spacing-xl)',
                background: 'var(--leo-color-systemfeedback-error-background)',
                borderRadius: 'var(--leo-radius-m)',
            }}>
                {error}
            </div>
        )
    }

    return (
        <div>
            <div style={{ marginBottom: 'var(--leo-spacing-xl)' }}>
                <h1 style={{
                    font: 'var(--leo-font-heading-h2)',
                    marginTop: 0,
                    marginBottom: 'var(--leo-spacing-s)',
                    color: 'var(--leo-color-text-primary)',
                }}>
                    {conversation?.title || 'Untitled Conversation'}
                </h1>
                <div style={{
                    display: 'flex',
                    alignItems: 'center',
                    gap: 'var(--leo-spacing-m)',
                }}>
                    <span style={{
                        fontFamily: 'monospace',
                        font: 'var(--leo-font-small-regular)',
                        color: 'var(--leo-color-text-tertiary)',
                    }}>
                        UUID: {conversation?.uuid || 'unknown'}
                    </span>
                    <RawToggleButton show={showConvRaw} onClick={() => setShowConvRaw(!showConvRaw)} />
                </div>
                {showConvRaw && <RawBlock data={conversation} />}
            </div>

            {turns.length === 0 ? (
                <div style={{
                    textAlign: 'center',
                    padding: 'var(--leo-spacing-4xl)',
                    color: 'var(--leo-color-text-tertiary)',
                }}>
                    No turns found.
                </div>
            ) : (
                <div style={{
                    overflow: 'hidden',
                }}>
                    {turns.map((turn, i) => <TurnView key={i} turn={turn} />)}
                </div>
            )}
        </div>
    )
}

function App() {
    const [conversations, setConversations] = React.useState<any[]>([])
    const [selectedUuid, setSelectedUuid] = React.useState<string | null>(() => {
        const params = new URLSearchParams(window.location.search)
        return params.get('uuid')
    })
    const [loading, setLoading] = React.useState(true)
    const [sidebarOpen, setSidebarOpen] = React.useState(false)
    const [isNarrow, setIsNarrow] = React.useState(false)

    React.useEffect(() => {
        const mql = window.matchMedia('(max-width: 979px)')
        const handler = (e: MediaQueryListEvent | MediaQueryList) => {
            setIsNarrow(e.matches)
            if (!e.matches) setSidebarOpen(false)
        }
        handler(mql)
        mql.addEventListener('change', handler)
        return () => mql.removeEventListener('change', handler)
    }, [])

    React.useEffect(() => {
        async function load() {
            try {
                const handler = Service.getRemote()
                const response = await handler.getConversations()
                const convs = response.conversations || []
                setConversations(convs)
                if (convs.length > 0 && !selectedUuid) {
                    setSelectedUuid(convs[0].uuid)
                }
            } catch {
            } finally {
                setLoading(false)
            }
        }
        load()
    }, [])


    const sidebarContent = (
        <>
            <div style={{
                padding: 'var(--leo-spacing-l) var(--leo-spacing-xl)',
                font: 'var(--leo-font-default-semibold)',
                flexShrink: 0,
                display: 'flex',
                justifyContent: 'space-between',
                alignItems: 'center',
                whiteSpace: 'nowrap',
            }}>
                Recent Chats
            </div>
            <div style={{ overflowY: 'auto', flex: 1 }}>
                {loading ? (
                    <div style={{
                        display: 'flex',
                        justifyContent: 'center',
                        padding: 'var(--leo-spacing-2xl)',
                    }}>
                        <ProgressRing />
                    </div>
                ) : conversations.length === 0 ? (
                    <div style={{
                        padding: 'var(--leo-spacing-xl)',
                        color: 'var(--leo-color-text-tertiary)',
                        textAlign: 'center',
                    }}>
                        No conversations found.
                    </div>
                ) : (
                    conversations.map((conv) => (
                        <ConversationNavItem
                            key={conv.uuid}
                            conv={conv}
                            selected={conv.uuid === selectedUuid}
                            onClick={() => {
                                setSelectedUuid(conv.uuid)
                                if (isNarrow) setSidebarOpen(false)
                            }}
                        />
                    ))
                )}
            </div>
        </>
    )

    return (
        <div style={{
            display: 'flex',
            flexDirection: 'column',
            height: '100vh',
            background: 'var(--leo-color-page-background)',
            color: 'var(--leo-color-text-primary)',
            font: 'var(--leo-font-default-regular)',
            position: 'relative',
        }}>
            <style>{`
                ::-webkit-scrollbar { width: 14px; }
                ::-webkit-scrollbar-track { background: transparent; }
                ::-webkit-scrollbar-thumb {
                    background-color: var(--leo-color-divider-subtle);
                    border: var(--leo-spacing-s) solid transparent;
                    background-clip: content-box;
                    border-radius: var(--leo-radius-m);
                }
                ::-webkit-scrollbar-thumb:hover { background-color: var(--leo-color-text-secondary); }
            `}</style>

            {isNarrow && sidebarOpen && (
                <div
                    onClick={() => setSidebarOpen(false)}
                    style={{
                        position: 'absolute',
                        inset: 0,
                        background: 'var(--leo-color-dialogs-scrim-background)',
                        zIndex: 10,
                    }}
                />
            )}

            {isNarrow && (
                <aside style={{
                    position: 'absolute',
                    top: 0,
                    left: 0,
                    height: '100%',
                    width: '280px',
                    zIndex: 20,
                    background: 'var(--leo-color-container-background)',
                    display: 'flex',
                    flexDirection: 'column',
                    transform: sidebarOpen ? 'translateX(0)' : 'translateX(-100%)',
                    transition: 'transform 200ms ease',
                    overflowY: 'auto',
                }}>
                    {sidebarContent}
                </aside>
            )}

            <div style={{
                display: 'flex',
                alignItems: 'center',
                gap: 'var(--leo-spacing-m)',
                padding: '0 var(--leo-spacing-l)',
                background: 'var(--leo-color-container-background)',
                flexShrink: 0,
                minHeight: '56px',
            }}>
                {isNarrow && (
                    <div style={{ opacity: sidebarOpen ? 0 : 1, pointerEvents: sidebarOpen ? 'none' : 'auto' }}>
                        <SidebarToggle open={sidebarOpen} onClick={() => setSidebarOpen(!sidebarOpen)} />
                    </div>
                )}
                <span style={{
                    font: 'var(--leo-font-heading-h3)',
                    color: 'var(--leo-color-text-primary)',
                }}>
                    AI Chat Internal
                </span>
            </div>

            <div style={{
                display: 'flex',
                flex: 1,
                overflow: 'hidden',
            }}>
                {!isNarrow && (
                    <aside style={{
                        width: '280px',
                        flexShrink: 0,
                        background: 'var(--leo-color-container-background)',
                        display: 'flex',
                        flexDirection: 'column',
                        height: '100%',
                        position: 'sticky',
                        top: 0,
                        overflowY: 'auto',
                    }}>
                        {sidebarContent}
                    </aside>
                )}

                <main style={{
                    flex: 1,
                    overflowY: 'auto',
                    minWidth: 0,
                }}>
                    <div style={{ padding: 'var(--leo-spacing-2xl)' }}>
                        {selectedUuid ? (
                            <ConversationDetail key={selectedUuid} uuid={selectedUuid} />
                        ) : (
                            <div style={{
                                display: 'flex',
                                justifyContent: 'center',
                                alignItems: 'center',
                                height: '100%',
                                color: 'var(--leo-color-text-tertiary)',
                            }}>
                                Select a conversation from the sidebar.
                            </div>
                        )}
                    </div>
                </main>
            </div>
        </div>
    )
}

const container = document.getElementById('app')
if (container) {
    document.body.style.margin = '0'
    document.body.style.padding = '0'
    document.body.style.overflow = 'hidden'
    const root = createRoot(container)
    root.render(<App />)
}
