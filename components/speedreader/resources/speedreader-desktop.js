/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// These IDs are hardcoded in extractor.rs
const showOriginalLinkId = 'c93e2206-2f31-4ddc-9828-2bb8e8ed940e'
const readTimeDivId = 'da24e4ef-db57-4b9f-9fa5-548924fc9c32'
const metaDataDivId = '3bafd2b4-a87d-4471-8134-7a9cca092000'
const contentDivId = '7c08a417-bf02-4241-a55e-ad5b8dc88f69'

const defaultSpeedreaderData = {
    showOriginalLinkText: 'View original',
    playButtonTitle: 'Play/Pause',
    averageWordsPerMinute: 265,
    minutesText: 'min. read',
    ttsEnabled: false,
}

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

const getTextContent = (element) => {
    if (!element) {
        return null
    }
    const text = (element.innerText || element.wholeText).replace(/\n|\r +/g, ' ').trim()
    if (text.length > 0) {
        return text
    }
    return null
}

const initTextToSpeak = () => {
    if (navigator.userAgentData.mobile || !speedreaderData.ttsEnabled) {
        return
    }

    let textToSpeak = 0

    const makeParagraph = (elem) => {
        if (!elem) {
            return false
        }
        const text = getTextContent(elem)
        if (text) {
            elem.setAttribute('tts-paragraph-index', textToSpeak++)
            return true
        }
        return false
    }

    const createPlayer = (p) => {
        const player = document.createElement('span')
        player.classList.add('tts-paragraph-player')
        const playButton = document.createElement('span')
        playButton.classList.add('tts-paragraph-player-button', 'tts-play-icon')
        playButton.title = speedreaderData.playButtonTitle
        playButton.onclick = (ev) => {
            window.speedreader.ttsPlayPause(parseInt(p.getAttribute('tts-paragraph-index')))
        }
        player.insertAdjacentElement('afterbegin', playButton)
        p.insertAdjacentElement('afterbegin', player)
    }

    makeParagraph($(metaDataDivId)?.querySelector('.title'))

    // Returns true if node is a leaf of the parentNode.
    const isChildOf = (node, parentNode) => {
        if (!parentNode) return false

        while (node) {
            if (node == parentNode) return true
            node = node.parentNode
        }
        return false
    }

    nodes = document.createNodeIterator(
        $(contentDivId),
        NodeFilter.SHOW_TEXT,
        { acceptNode(n) { return NodeFilter.FILTER_ACCEPT } })

    let currentParent = null;
    let paragraphs = []
    while ((currentNode = nodes.nextNode())) {
        if (currentNode === nodes.root ||
            isChildOf(currentNode, currentParent) ||
            !getTextContent(currentNode)) {
            continue
        }
        currentParent = currentNode.parentNode

        // Remove all nodes that are children of the current node.
        // It is only possible when a paragraph starts with <tag>text</tag>,
        // in this case NodeIterator produces sequence <tag> -> <p>,
        // but <p> is a parent of the <tag>, so we add <p> and remove <tag>
        paragraphs = paragraphs.filter((p) => {
            return !isChildOf(p, currentParent)
        })
        paragraphs.push(currentNode.parentElement)
    }

    paragraphs.forEach((p) => {
        if (makeParagraph(p)) {
            createPlayer(p)
        }
    })
}

const extractTextToSpeak = () => {
    const paragraphs = Array.from(document.querySelectorAll('[tts-paragraph-index]'))
        .map((p) => {
            return getTextContent(p)
        })

    return {
        title: document.title,
        author: $(metaDataDivId)?.querySelector('.author')?.textContent,
        desciption: $(metaDataDivId)?.querySelector('.subhead')?.textContent,
        paragraphs: paragraphs
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
    }
}

const main = () => {
    // SpeedreaderTabHelper may override 'speedreaderData' with localized data.
    window.speedreaderData = Object.assign(defaultSpeedreaderData, window.speedreaderData)

    initShowOriginalLink()
    calculateReadtime()
    initTextToSpeak()
}

(() => { main() })()
