# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Uses Leo through its UI: chats in brave://leo-ai, and the memories in
brave://settings/leo-ai/customization."""

import json
import time

from cdp import FIND, WebSocket, find_in, log, targets

LEO_INPUT = (
    f'(()=>{{const f={FIND};return f(e=>e.isContentEditable&&'
    "e.getAttribute('data-testid')==='leo-input')})()"
)

# Leo's first-use dialog shows when the first message is sent.
ACCEPT_DISCLAIMER = (
    '(()=>{const out=[];const walk=r=>{for(const e of r.querySelectorAll('
    "'*')){if(e.tagName==='LEO-BUTTON'&&(e.textContent||'').trim()==="
    "'I understand'&&e.getClientRects().length>0)out.push(e);"
    'if(e.shadowRoot)walk(e.shadowRoot)}};walk(document);'
    'if(!out.length)return false;out[0].click();return true})()'
)

MEMORY_SECTION = 'MEMORY-SECTION'


class Conversation:
    """One brave://leo-ai tab, which is one conversation."""

    def __init__(self, browser):
        self.browser = browser
        self.ws, self.tab = browser.new_tab('brave://leo-ai/', settle=6)

    def frame(self):
        frames = [
            t
            for t in targets()
            if 'conversation-entries' in t['url']
            and t.get('parentId') == self.tab
        ]
        return WebSocket(frames[0]['webSocketDebuggerUrl']) if frames else None

    def text(self):
        try:
            frame = self.frame()
            return frame.eval('document.body.innerText') if frame else ''
        except (OSError, RuntimeError):
            return ''

    def send(self, message):
        """Types |message| into the chat box, as a user, and presses Enter."""
        typed = None
        for _ in range(3):
            # Select what the box holds, so the message replaces it.
            self.ws.eval(
                f'(()=>{{const el={LEO_INPUT};el.focus();'
                "document.execCommand('selectAll');return 1})()"
            )
            time.sleep(0.5)
            self.ws.call('Input.insertText', text=message)
            time.sleep(0.5)
            typed = self.ws.eval(f'{LEO_INPUT}.innerText.trim()')
            if typed == message:
                break
        else:
            raise RuntimeError(f'the chat box holds {typed!r}, not the message')
        for kind in ('keyDown', 'keyUp'):
            self.ws.call(
                'Input.dispatchKeyEvent',
                type=kind,
                key='Enter',
                code='Enter',
                windowsVirtualKeyCode=13,
            )

    def accept_disclaimer(self):
        return self.ws.eval(ACCEPT_DISCLAIMER)

    def close(self):
        self.browser.close_tab(self.tab)


def _reply_length(text, sent):
    """The characters of the conversation that the user did not type."""
    return len(text) - sum(len(m) for m in sent)


def _wait_for_reply(conversation, sent, before, limit=300, stable=8):
    """Waits until a reply comes and then stops changing. A search or a tool
    call can keep the text still for a while before the reply."""
    start, last, since = time.time(), None, time.time()
    while time.time() - start < limit:
        text = conversation.text()
        added = _reply_length(text, sent)
        if text != last:
            last, since = text, time.time()
        elif added - before > 40 and time.time() - since >= stable:
            return text, added
        time.sleep(1)
    return last or '', _reply_length(last or '', sent)


def chat(browser, messages, tries=3):
    """Types |messages| into one new conversation, and gives the replies.
    Each message must get a reply from Leo, or it is sent again."""
    conversation = Conversation(browser)
    sent, replies = [], []
    try:
        for message in messages:
            before = _reply_length(conversation.text(), sent)
            for attempt in range(1, tries + 1):
                conversation.send(message)
                time.sleep(2)
                if conversation.accept_disclaimer():
                    time.sleep(2)
                text, added = _wait_for_reply(
                    conversation, sent + [message], before
                )
                if added - before > 40:
                    break
                log(f'    no reply to {message[:40]!r} (try {attempt})')
                if message in text:
                    # The message is in the chat; a new try would repeat it.
                    raise RuntimeError(f'Leo did not reply to {message!r}')
            else:
                raise RuntimeError(f'could not send {message!r}')
            sent.append(message)
            replies.append(text.rsplit(message, 1)[-1].strip())
            log(f'    {message[:60]!r}: {added - before} characters of reply')
    finally:
        conversation.close()
    return replies


def open_customization(browser):
    return browser.new_tab('brave://settings/leo-ai/customization', settle=6)


def memories(ws):
    """The memories that the user wrote."""
    return ws.eval(
        find_in(MEMORY_SECTION, 'return sec.memoriesList_;', visible=False)
    )


def add_memory(ws, text):
    ws.eval(
        find_in(
            MEMORY_SECTION,
            f'sec.saveMemory_({json.dumps(text)}, null); return 1;',
            visible=False,
        )
    )


def delete_memory(ws, text):
    ws.eval(
        find_in(
            MEMORY_SECTION,
            'sec.browserProxy_.getCustomizationSettingsHandler()'
            f'.deleteMemory({json.dumps(text)}); return 1;',
            visible=False,
        )
    )


def search_memories(ws, query):
    """The memories that the settings search finds, both kinds, the best
    first: [{'text', 'learned'}]."""
    return ws.eval(
        find_in(
            MEMORY_SECTION,
            'const r = await sec.browserProxy_'
            '.getCustomizationSettingsHandler()'
            f'.searchMemories({json.dumps(query)});'
            'return (r.results || []).map(m => ({text: m.text,'
            ' learned: !!m.learnedUuid}));',
            visible=False,
        )
    )
