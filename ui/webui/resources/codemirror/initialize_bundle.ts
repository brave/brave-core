// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// ESM facade for the shared CodeMirror bundle. It is served as
// chrome://resources/brave/codemirror.bundle.js and only registers the
// <brave-code-editor> custom element.

import { Compartment, EditorState } from '@codemirror/state'
import { EditorView, dropCursor, highlightSpecialChars, keymap } from '@codemirror/view'
import {
  defaultKeymap,
  history,
  historyKeymap,
  indentWithTab,
} from '@codemirror/commands'

// Apply default <textarea> styling
const theme = EditorView.theme({
  '&': {
    color: 'inherit',
    backgroundColor: 'transparent',
    height: '100%',
    fontFamily: 'monospace',
    fontSize: '13px',
  },
  '&.cm-focused': {
    outline: 'none',
  },
  '.cm-content': {
    lineHeight: '1.5',
    caretColor: 'inherit',
  },
  '.cm-dropCursor': {
    borderLeftColor: 'currentColor',
  },
})

const editorExtensions = (readonly: boolean) => [
  EditorState.readOnly.of(readonly),
  EditorView.editable.of(!readonly),
]

class BraveCodeEditor extends HTMLElement {
  #view: EditorView | undefined
  #value = ''
  #readonly = false
  #readOnlyCompartment = new Compartment()

  get value(): string {
    return this.#value
  }

  set value(value: string) {
    // Avoid needless dispatches, which also breaks the update cycle when
    // changes originated from our own update listener
    if (this.#value === value) {
      return
    }
    this.#value = value
    if (this.#view) {
      const length = this.#view.state.doc.length
      this.#view.dispatch({ changes: { from: 0, to: length, insert: value } })
    }
  }

  get readonly(): boolean {
    return this.#readonly
  }

  set readonly(readonly: boolean) {
    this.#readonly = readonly
    this.toggleAttribute('readonly', readonly)
    if (this.#view) {
      this.#view.dispatch({
        effects: this.#readOnlyCompartment.reconfigure(
          editorExtensions(readonly),
        ),
      })
    }
  }

  connectedCallback() {
    if (this.#view) {
      return
    }
    const root = this.attachShadow({ mode: 'open' })
    const style = document.createElement('style')
    style.textContent = `
      :host {
        display: block;
        width: 100%;
        height: 100%;
      }
      :host([readonly]) .cm-editor {
        opacity: .5;
      }
    `
    root.appendChild(style)
    this.#view = new EditorView({
      parent: root,
      state: EditorState.create({
        doc: this.#value,
        extensions: [
          this.#readOnlyCompartment.of(editorExtensions(this.#readonly)),
          dropCursor(),
          highlightSpecialChars(),
          history(),
          keymap.of([indentWithTab, ...defaultKeymap, ...historyKeymap]),
          EditorView.lineWrapping,
          EditorView.contentAttributes.of({ spellcheck: 'false' }),
          theme,
          EditorView.updateListener.of((update) => {
            if (!update.docChanged) {
              return
            }
            this.#value = update.state.doc.toString()
            this.dispatchEvent(
              new CustomEvent('code-change', {
                detail: { value: this.#value },
                bubbles: true,
                composed: true,
              }),
            )
          }),
        ],
      }),
    })
  }
}

customElements.define('brave-code-editor', BraveCodeEditor)

export { BraveCodeEditor }
