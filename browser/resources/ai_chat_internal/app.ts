// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { AIChatInternalPageHandler } from './ai_chat_internal.mojom-webui.js';
import { CharacterType } from './common.mojom-webui.js';

function formatTimestamp(mojoTime: any): string {
    if (!mojoTime) return 'Unknown';
    try {
        const windowsToUnixEpochUs = BigInt(11644473600) * BigInt(1000000);
        const unixMicros = BigInt(mojoTime.internalValue) - windowsToUnixEpochUs;
        const unixMs = Number(unixMicros / BigInt(1000));
        return new Date(unixMs).toLocaleString();
    } catch {
        return String(mojoTime);
    }
}

function rawStringify(obj: any): string {
    return JSON.stringify(obj, (_key, value) =>
        typeof value === 'bigint' ? value.toString() + 'n' : value, 2);
}

function renderConversationDetail(contentDiv: HTMLElement, response: any) {
    contentDiv.textContent = '';

    const conv = response.conversation;
    const turns = response.entries || [];

    const convHeader = document.createElement('div');
    convHeader.style.display = 'flex';
    convHeader.style.justifyContent = 'space-between';
    convHeader.style.alignItems = 'flex-start';

    const convHeaderLeft = document.createElement('div');

    const h1 = document.createElement('h1');
    h1.textContent = conv?.title || 'Untitled Conversation';
    convHeaderLeft.appendChild(h1);

    const meta = document.createElement('p');
    meta.textContent = `UUID: ${conv?.uuid || 'unknown'}`;
    meta.style.color = '#666';
    meta.style.fontFamily = 'monospace';
    convHeaderLeft.appendChild(meta);

    convHeader.appendChild(convHeaderLeft);

    // Raw conversation metadata toggle
    const convRawBtn = document.createElement('a');
    convRawBtn.href = '#';
    convRawBtn.textContent = 'View Raw';
    convRawBtn.style.color = '#0052cc';
    convRawBtn.style.textDecoration = 'none';
    convRawBtn.style.marginLeft = '8px';
    convRawBtn.style.fontWeight = 'bold';
    convRawBtn.title = 'View Raw Conversation Metadata';

    meta.appendChild(convRawBtn);

    contentDiv.appendChild(convHeader);

    const convRawPre = document.createElement('pre');
    convRawPre.textContent = rawStringify(conv);
    convRawPre.style.display = 'none';
    convRawPre.style.background = '#f4f4f4';
    convRawPre.style.padding = '8px';
    convRawPre.style.fontSize = '0.8em';
    convRawPre.style.overflowX = 'auto';
    convRawPre.style.border = '1px solid #ccc';
    contentDiv.appendChild(convRawPre);

    convRawBtn.addEventListener('click', (e) => {
        e.preventDefault();
        const hidden = convRawPre.style.display === 'none';
        convRawPre.style.display = hidden ? 'block' : 'none';
        convRawBtn.textContent = hidden ? 'Hide Raw' : 'View Raw';
    });

    const back = document.createElement('a');
    back.href = 'chrome://ai-chat-internal/';
    back.textContent = 'Back to conversations';
    back.style.display = 'block';
    back.style.marginBottom = '8px';
    contentDiv.appendChild(back);

    if (turns.length === 0) {
        const p = document.createElement('p');
        p.textContent = 'No turns found.';
        contentDiv.appendChild(p);
        return;
    }

    for (const turn of turns) {
        const div = document.createElement('div');
        div.style.marginBottom = '16px';
        div.style.padding = '8px';
        div.style.border = '1px solid #ccc';
        div.style.borderRadius = '4px';

        const header = document.createElement('div');
        header.style.display = 'flex';
        header.style.justifyContent = 'space-between';
        header.style.alignItems = 'flex-start';
        header.style.marginBottom = '4px';

        const headerLeft = document.createElement('div');
        headerLeft.style.display = 'flex';
        headerLeft.style.alignItems = 'baseline';
        headerLeft.style.gap = '8px';

        const role = document.createElement('strong');
        role.textContent = turn.characterType === CharacterType.HUMAN ? 'User' : 'Assistant';
        headerLeft.appendChild(role);

        const time = document.createElement('span');
        time.textContent = formatTimestamp(turn.createdTime);
        time.style.color = '#666';
        time.style.fontSize = '0.85em';

        // Raw turn data toggle
        const rawBtn = document.createElement('a');
        rawBtn.href = '#';
        rawBtn.textContent = 'View Raw';
        rawBtn.style.color = '#0052cc';
        rawBtn.style.textDecoration = 'none';
        rawBtn.style.marginLeft = '8px';
        rawBtn.style.fontWeight = 'bold';
        rawBtn.title = 'View Raw Turn Data';

        time.appendChild(rawBtn);
        headerLeft.appendChild(time);

        header.appendChild(headerLeft);

        div.appendChild(header);

        const text = document.createElement('p');
        text.textContent = turn.text || '';
        text.style.marginTop = '4px';
        text.style.whiteSpace = 'pre-wrap';
        div.appendChild(text);

        const rawPre = document.createElement('pre');
        rawPre.textContent = rawStringify(turn);
        rawPre.style.display = 'none';
        rawPre.style.marginTop = '6px';
        rawPre.style.background = '#f4f4f4';
        rawPre.style.padding = '8px';
        rawPre.style.fontSize = '0.75em';
        rawPre.style.overflowX = 'auto';
        rawPre.style.border = '1px solid #ccc';
        rawPre.style.borderRadius = '4px';
        div.appendChild(rawPre);

        rawBtn.addEventListener('click', (e) => {
            e.preventDefault();
            const hidden = rawPre.style.display === 'none';
            rawPre.style.display = hidden ? 'block' : 'none';
            rawBtn.textContent = hidden ? 'Hide Raw' : 'View Raw';
        });

        contentDiv.appendChild(div);
    }
}

async function init() {
    const handler = AIChatInternalPageHandler.getRemote();
    const contentDiv = document.getElementById('content');
    if (!contentDiv) return;

    const urlParams = new URLSearchParams(window.location.search);
    const uuid = urlParams.get('uuid');

    if (uuid) {
        contentDiv.textContent = 'Fetching conversation data...';
        try {
            const response = await handler.getRawConversationData(uuid);
            renderConversationDetail(contentDiv, response);
        } catch (e) {
            contentDiv.textContent = `Error fetching data: ${e}`;
        }
    } else {
        contentDiv.textContent = 'Fetching conversation list...';
        try {
            const response = await handler.getConversations();
            const conversations = response.conversations || [];
            if (conversations.length === 0) {
                contentDiv.textContent = 'No conversations found.';
                return;
            }

            contentDiv.textContent = '';
            const h1 = document.createElement('h1');
            h1.textContent = 'AI Chat Conversations';
            contentDiv.appendChild(h1);

            const ul = document.createElement('ul');
            for (const conv of conversations) {
                const li = document.createElement('li');
                const a = document.createElement('a');
                a.href = `?uuid=${encodeURIComponent(conv.uuid)}`;
                a.textContent = `${conv.title || 'Untitled'} (${conv.uuid})`;
                li.appendChild(a);
                ul.appendChild(li);
            }
            contentDiv.appendChild(ul);
        } catch (e) {
            contentDiv.textContent = `Error fetching list: ${e}`;
        }
    }
}

document.addEventListener('DOMContentLoaded', init);
