/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

class speedreaderUtils {
    // These IDs are hardcoded in extractor.rs
    static showOriginalLinkId = 'c93e2206-2f31-4ddc-9828-2bb8e8ed940e'
    static readTimeDivId = 'da24e4ef-db57-4b9f-9fa5-548924fc9c32'
    static metaDataDivId = '3bafd2b4-a87d-4471-8134-7a9cca092000'
    static contentDivId = '7c08a417-bf02-4241-a55e-ad5b8dc88f69'

    static defaultSpeedreaderData = {
        showOriginalLinkText: 'View original',
        playButtonTitle: 'Play / Pause',
        averageWordsPerMinute: 265,
        minutesText: 'min. read',
        ttsEnabled: false,
        ttsReading: false,
    }

    static $ = (id) => {
        return document.getElementById(id)
    }

    static adoptStyles = () => {
        for (const style of document.styleSheets) {
            style.disabled = true
        }

        const braveStyles =
            document.querySelectorAll('script[type="brave-style-data"]')
        for (const styleData of braveStyles) {
            const style = new CSSStyleSheet()
            style.replaceSync(styleData.innerText)
            document.adoptedStyleSheets.push(style)
        }
        document.body.hidden = false
    }

    static initShowOriginalLink = () => {
        const link = this.$(this.showOriginalLinkId)
        if (!link)
            return

        link.innerText = this.speedreaderData.showOriginalLinkText
        link.addEventListener('click', (e) => {
            window.speedreader.showOriginalPage()
        })
        window.addEventListener('keydown', (event) => {
            if (event.key === 'Escape' &&
                !event.altKey && !event.shiftKey &&
                !event.metaKey && !event.ctrlKey) {
                window.speedreader.showOriginalPage()
                event.preventDefault()
                event.stopPropagation()
            }
        })
    }

    static calculateReadtime = () => {
        const readTimeDiv = this.$(this.readTimeDivId)
        if (!readTimeDiv)
            return

        const text = document.body.innerText
        const words = text.trim().split(/\s+/).length
        const wpm = this.speedreaderData.averageWordsPerMinute
        const minutes = Math.ceil(words / wpm)

        readTimeDiv.innerText = minutes + ' ' + speedreaderData.minutesText
    }

    static getTextContent = (element) => {
        if (!element) {
            return null
        }
        const text = (element.innerText || element.wholeText)?.replace(/\n|\r +/g, ' ')?.trim()
        if (text?.length > 0) {
            return text
        }
        return null
    }

    static isTtsEnabled() {
        return !navigator.userAgentData.mobile || this.speedreaderData.ttsEnabled
    }

    static initTextToSpeak = () => {
        if (!this.isTtsEnabled()) {
            return
        }

        {
            const spans =
                this.$(this.contentDivId)?.querySelectorAll('p > span')

            for (const span of spans) {
                const p = span.parentNode

                while (span.childNodes.length > 0) {
                    p.insertBefore(span.firstChild, span)
                }
                p.removeChild(span)
            }
        }

        let textToSpeak = 0

        const makeParagraph = (elem) => {
            if (!elem) {
                return false
            }
            const text = this.getTextContent(elem)
            if (text) {
                elem.setAttribute('tts-paragraph-index', textToSpeak++)
                return true
            }
            return false
        }

        const createPlayer = (p) => {
            const player = document.createElement('span')
            player.classList.add('tts-paragraph-player')
            const button = document.createElement('span')
            button.classList.add('tts-circle')
            const playButton = document.createElement('span')
            playButton.classList.add('tts-paragraph-player-button', 'tts-play-icon')
            playButton.title = speedreaderData.playButtonTitle
            playButton.onclick = button.onclick = (ev) => {
                window.speedreader.ttsPlayPause(parseInt(p.getAttribute('tts-paragraph-index')))
            }
            player.insertAdjacentElement('afterbegin', playButton)
            player.insertAdjacentElement('afterbegin', button)
            p.insertAdjacentElement('afterbegin', player)
        }

        makeParagraph(this.$(this.metaDataDivId)?.querySelector('.title'))
        makeParagraph(this.$(this.metaDataDivId)?.querySelector('.subhead'))

        // Returns true if node is a leaf of the parentNode.
        const isChildOf = (node, parentNode) => {
            if (!parentNode) return false

            while (node) {
                if (node == parentNode) return true
                node = node.parentNode
            }
            return false
        }

        const nodes = document.createNodeIterator(
            this.$(this.contentDivId),
            NodeFilter.SHOW_TEXT,
            { acceptNode(n) { return NodeFilter.FILTER_ACCEPT } })

        let currentParent = null
        let paragraphs = []
        let currentNode = null;
        while (currentNode = nodes.nextNode()) {
            if (currentNode === nodes.root ||
                isChildOf(currentNode, currentParent) ||
                !this.getTextContent(currentNode)) {
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

        const underline = document.createElement('div')
        underline.classList.add('tts-underline')
        document.body.insertAdjacentElement('beforeend', underline)
    }

    static extractTextToSpeak = () => {
        const paragraphs = Array.from(document.querySelectorAll('[tts-paragraph-index]'))
            .map((p) => {
                return p.textContent
            })

        return {
            title: document.title,
            author: this.$(this.metaDataDivId)?.querySelector('.author')?.textContent,
            desciption: this.$(this.metaDataDivId)?.querySelector('.subhead')?.textContent,
            paragraphs: paragraphs
        }
    }

    static highlightWord = (paragraph, start, end) => {
        if (!paragraph || start >= end) {
            CSS.highlights.clear()
            const underline = document.querySelector('.tts-underline')
            underline.classList.remove('tts-underline-visible')
            underline.setAttribute('data-top', 0)
            return
        }

        const nodes = document.createNodeIterator(
            paragraph,
            NodeFilter.SHOW_TEXT,
            { acceptNode(n) { return NodeFilter.FILTER_ACCEPT } })

        const range = new Range()

        let startNode = null
        let endNode = null
        let node = null
        while (node = nodes.nextNode()) {
            if (!startNode) {
                if (start < node.textContent.length) {
                    startNode = node
                    range.setStart(node, start)
                }
                start -= node.textContent.length
            }
            if (!endNode) {
                if (end <= node.textContent.length) {
                    endNode = node
                    range.setEnd(endNode, end)
                }
                end -= node.textContent.length
            }
        }

        const underline = document.querySelector('.tts-underline')
        const bodyRect = document.body.getBoundingClientRect()
        const highlightedRect = range.getBoundingClientRect()

        const top = highlightedRect.bottom - bodyRect.top
        const left = highlightedRect.left - bodyRect.left
        const width = highlightedRect.width + 2;

        underline.classList.toggle('tts-underline-newline',
            underline.getAttribute('data-top') != top)
        underline.classList.toggle('tts-underline-decrease',
            underline.getAttribute('data-width') < width)
        underline.classList.add('tts-underline-visible')
        underline.setAttribute('data-top', top)
        underline.setAttribute('data-width', width)

        underline.style.setProperty('--tts-underline-top', top + 'px');
        underline.style.setProperty('--tts-underline-left', left + 'px');
        underline.style.setProperty('--tts-underline-width', width + 'px');

        CSS.highlights.set('tts-highlighted-word', new Highlight(range))
    }

    static setTtsButtonIcon = (e, i) => {
        const button = e?.querySelector('.tts-paragraph-player-button')
        if (button) {
            button.classList.remove('tts-play-icon', 'tts-pause-icon')
            button.classList.add(i)
        }
    }

    static highlightText = (ttsParagraphIndex, charIndex, length) => {
        if (!this.isTtsEnabled()) {
            return
        }

        document.querySelectorAll('.tts-highlighted').forEach((e) => {
            if (e.getAttribute('tts-paragraph-index') != ttsParagraphIndex) {
                e.classList.remove('tts-highlighted')
                this.setTtsButtonIcon(e, 'tts-play-icon')
            }
        })

        const paragraph = document.querySelector(
            '[tts-paragraph-index="' + ttsParagraphIndex + '"]')
        if (paragraph) {
            paragraph.classList.add('tts-highlighted')
            if (this.speedreaderData.ttsReading) {
                this.setTtsButtonIcon(paragraph, 'tts-pause-icon')
            } else {
                this.setTtsButtonIcon(paragraph, 'tts-play-icon')
            }
        }
        this.highlightWord(paragraph, charIndex, charIndex + length)
    }

    static setTtsReadingState = (reading) => {
        this.speedreaderData.ttsReading = reading
        if (!this.speedreaderData.ttsReading) {
            this.setTtsButtonIcon(document.querySelector('.tts-highlighted'), 'tts-play-icon')
        }
    }
}

(() => {
    speedreaderUtils.speedreaderData = Object.assign(
        speedreaderUtils.defaultSpeedreaderData,
        window.speedreaderData)

    speedreaderUtils.adoptStyles()
    speedreaderUtils.initShowOriginalLink()
    speedreaderUtils.calculateReadtime()
    speedreaderUtils.initTextToSpeak()
})()
