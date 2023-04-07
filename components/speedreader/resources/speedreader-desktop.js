/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// These IDs are hardcoded in extractor.rs
const showOriginalLinkId = 'c93e2206-2f31-4ddc-9828-2bb8e8ed940e'
const readTimeDivId = 'da24e4ef-db57-4b9f-9fa5-548924fc9c32'

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

const defaultSpeedreaderData = {
    showOriginalLinkText: 'View original',
    averageWordsPerMinute: 265,
    minutesText: 'min. read',
}

const main = () => {
    // SpeedreaderTabHelper may override 'speedreaderData' with localized data.
    window.speedreaderData = Object.assign(defaultSpeedreaderData, window.speedreaderData)

    initShowOriginalLink()
    calculateReadtime()
}

(() => { main() })()
