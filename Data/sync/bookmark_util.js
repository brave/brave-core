// This is a slightly modified file from sync framework to make it work with `JavaScriptCore`
// IMPORTANT: This file needs to be updated
// each time `https://github.com/brave/sync/blob/staging/client/bookmarkUtil.js` is updated.
// The only difference in this file is replacing `module.exports.` with `var` in functions.
// Function bodies and signatures are the same as original.
// For original source please check https://github.com/brave/sync

'use strict'


/**
 * Returns a base bookmark order based on deviceId and platform.
 * @param {String} deviceId
 * @param {String} platform
 * @returns {String}
 */
var getBaseBookmarksOrder = (deviceId, platform) => {
    return `${(platform === 'ios' || platform === 'android') ? 2 : 1}.${deviceId}.`
}

/**
 * Helper to get next order number.
 * @param {number} lastNumber - last digit of prev order
 * @param {string} order - the order string
 * @return {string}
 */
const getNextOrderFromPrevOrder = (lastNumber, order) => {
    if (lastNumber <= 0) {
        throw new Error('Invalid input order')
    } else {
        return order + (lastNumber + 1)
    }
}

/**
 * Helper to get previous order number.
 * @param {number} lastNumber - last digit of next order
 * @param {string} order - the order string
 * @return {string}
 */
const getPrevOrderFromNextOrder = (lastNumber, order) => {
    if (lastNumber <= 0) {
        throw new Error('Invalid input order')
    } else if (lastNumber === 1) {
        return order + '0.1'
    } else {
        return order + (lastNumber - 1)
    }
}

/**
 * Returns current bookmark order based on previous and next bookmark order.
 * @param {String} prevOrder
 * @param {String} nextOrder
 * @returns {String}
 */
var getBookmarkOrder = (prevOrder, nextOrder) => {
    let prevOrderSplit = prevOrder.split('.')
    let nextOrderSplit = nextOrder.split('.')
    if (prevOrderSplit.length === 1 && nextOrderSplit.length === 1) {
        throw new Error(`Invalid previous and next orders: ${prevOrderSplit} and ${nextOrderSplit}`)
    }
    let order = ''
    if (nextOrderSplit.length === 1) {
        // Next order is an empty string
        if (prevOrderSplit.length > 2) {
            for (var i = 0; i < prevOrderSplit.length - 1; i++) {
                order += prevOrderSplit[i] + '.'
            }
            let lastNumber = parseInt(prevOrderSplit[prevOrderSplit.length - 1])
            order = getNextOrderFromPrevOrder(lastNumber, order)
        }
    } else if (prevOrderSplit.length === 1) {
        // Prev order is an empty string
        if (nextOrderSplit.length > 2) {
            for (i = 0; i < nextOrderSplit.length - 1; i++) {
                order += nextOrderSplit[i] + '.'
            }
            let lastNumber = parseInt(nextOrderSplit[nextOrderSplit.length - 1])
            order = getPrevOrderFromNextOrder(lastNumber, order)
        }
    } else {
        if (prevOrderSplit.length > 2 && nextOrderSplit.length > 2) {
            for (i = 0; i < prevOrderSplit.length - 1; i++) {
                order += prevOrderSplit[i] + '.'
            }
            if (prevOrderSplit.length === nextOrderSplit.length) {
                // Orders have the same length
                let lastNumberPrev = parseInt(prevOrderSplit[prevOrderSplit.length - 1])
                let lastNumberNext = parseInt(nextOrderSplit[nextOrderSplit.length - 1])
                if (lastNumberNext - lastNumberPrev > 1) {
                    order += (lastNumberPrev + 1)
                } else {
                    order += lastNumberPrev + '.1'
                }
            } else if (prevOrderSplit.length < nextOrderSplit.length) {
                // Next order is longer than previous order
                let lastNumberDiff = true
                for (i = 0; i < prevOrderSplit.length - 1; i++) {
                    if (prevOrderSplit[i] === nextOrderSplit[i]) {
                        continue
                    }
                    lastNumberDiff = false
                    break
                }
                order += prevOrderSplit[prevOrderSplit.length - 1] + '.'
                let currentIndex = prevOrderSplit.length
                while (parseInt(nextOrderSplit[currentIndex]) === 0) {
                    order += nextOrderSplit[currentIndex] + '.'
                    currentIndex++
                }
                let lastNumberNext = parseInt(nextOrderSplit[currentIndex])
                let lastNumberPrev = parseInt(prevOrderSplit[prevOrderSplit.length - 1])
                if (lastNumberDiff) {
                    let samePositionNumberNext = parseInt(nextOrderSplit[prevOrderSplit.length - 1])
                    if ((samePositionNumberNext - lastNumberPrev) >= 1) {
                        order += '1'
                    } else {
                        order = getPrevOrderFromNextOrder(lastNumberNext, order)
                    }
                } else {
                    order = getPrevOrderFromNextOrder(lastNumberNext, order)
                }
            } else {
                // Prev order is longer than next order
                let lastNumber = parseInt(prevOrderSplit[prevOrderSplit.length - 1])
                order = getNextOrderFromPrevOrder(lastNumber, order)
            }
        }
    }
    return order
}
