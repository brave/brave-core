/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// These IDs are hardcoded in extractor.rs
const showOriginalLinkId = 'c93e2206-2f31-4ddc-9828-2bb8e8ed940e'
const readTimeDivId = 'da24e4ef-db57-4b9f-9fa5-548924fc9c32'
const metaDataDivId = '3bafd2b4-a87d-4471-8134-7a9cca092000'
const contentDivId = '7c08a417-bf02-4241-a55e-ad5b8dc88f69'

const $ = (id) => {
    return document.getElementById(id)
}

const initShowOriginalLink = () => {
    const link = $(showOriginalLinkId)
    if (!link)
        return

    link.innerText = speedreaderData.showOriginalLinkText
    link.addEventListener('click', (e) => {
        window.speedreader.showOriginalPage()
    })

    if (!navigator.userAgentData.mobile) {
        link.style.display = 'none'
    }
}

const calculateReadtime = () => {
    const readTimeDiv = $(readTimeDivId)
    if (!readTimeDiv)
        return

    const text = document.body.innerText
    const words = text.trim().split(/\s+/).length
    const wpm = speedreaderData.averageWordsPerMinute
    const minutes = Math.ceil(words / wpm)

    readTimeDiv.innerText = minutes + ' ' + speedreaderData.minutesText
}

const defaultSpeedreaderData = {
    showOriginalLinkText: 'View original',
    averageWordsPerMinute: 265,
    minutesText: 'min. read',
}

const extractTextToSpeak = () => {
    const textTags = ['P', 'DIV', 'MAIN', 'ARTICLE']

    const extractParagraphs = (node) => {
        let paragraphs = []
        if (!node) {
            return paragraphs
        }
        for (const child of node.children) {
            if (textTags.indexOf(child.tagName) >= 0) {
                const childParagraphs = extractParagraphs(child)
                if (childParagraphs.length == 0) {
                    paragraphs.push(child)
                } else {
                    paragraphs = paragraphs.concat(childParagraphs)
                }
            }
        }
        return paragraphs
    }

    const paragraphs = extractParagraphs($(contentDivId))

    const textToSpeak = []
    for (const p of paragraphs) {
        const text = p.innerText.replace(/\n|\r +/g, ' ').trim()
        if (text) {
            p.setAttribute('tts-paragraph-index', textToSpeak.length)
            textToSpeak.push(p.innerText.replace(/\n|\r +/g, ' '))
        }
    }

    return {
        title: document.title,
        author: $(metaDataDivId)?.querySelector('.author')?.textContent,
        desciption: $(metaDataDivId)?.querySelector('.subhead')?.textContent,
        paragraphs: textToSpeak
    }
}

const highlightText = (ttsParagraphIndex, charIndex, length) => {
    document.querySelectorAll('.tts-highlighted').forEach((e) => {
        if (e.getAttribute('tts-paragraph-index') != ttsParagraphIndex) {
            e.classList.remove('tts-highlighted')
        }
    })

    const paragraph = document.querySelector(
        '[tts-paragraph-index="' + ttsParagraphIndex + '"]')
    if (paragraph) {
        paragraph.classList.add('tts-highlighted')
        const ttsContent = document.createElement('tts-content')
        ttsContent.append(...paragraph.childNodes)
        paragraph.append(ttsContent)
        document.documentElement.style.setProperty('--tts-highlight-progress', (charIndex / paragraph.textContent.length) * 100 + '%')
    }
}

const main = () => {
    // SpeedreaderTabHelper may override 'speedreaderData' with localized data.
    window.speedreaderData = Object.assign(defaultSpeedreaderData, window.speedreaderData)

    initShowOriginalLink()
    calculateReadtime()
}

(() => { main() })()
