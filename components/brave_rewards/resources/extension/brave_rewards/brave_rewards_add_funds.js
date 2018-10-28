/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

function init() {
    console.log(window.location.search)
    let addresses = []
    try {
        const urlParams = new URLSearchParams(window.location.search);
        const query_value = urlParams.get('addresses')
        const json = window.atob(decodeURIComponent(query_value))
        const addresses_dictionary = JSON.parse(json)
        for (let key in addresses_dictionary)
            addresses.push(addresses_dictionary[key])
        console.log(addresses)
    } catch (e) {
        console.error('Error parsing incoming args', window.location.search, e)
    }

    sendMessage({
        type: 'init',
        data: {
            denomination: {
                amount: 5,
                currency: 'USD'
            },
            destination: 'be41061a-8fc0-4c15-bb67-736345bdc321',
            addresses: addresses
        }
    })
}

function receiveMessage(event) {
    // Add origin check inside event.origin
    console.log(event)
}

function sendMessage(data) {
    uphold.contentWindow.postMessage(JSON.stringify(data), '*')
}

window.onload = function () {
    window.addEventListener("message", receiveMessage)
    console.log('init')
    init()
}

