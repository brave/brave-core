const messageHandler = require('./ipfs_onboarding.js')


test('Validate origin success', () => {
  expect(messageHandler({ 'type': 'message', 'origin': document.location.origin, 'data': { 'command': 'ipfs' } })).toStrictEqual(true)
})

test('Do not handle * origin', () => {
  expect(messageHandler({ 'type': 'message', 'origin': '*', 'data': { 'command': 'ipfs' } })).toStrictEqual(false)
})

test('Do not handle null origin', () => {
  expect(messageHandler({ 'type': 'message', 'origin': 'null', 'data': { 'command': 'ipfs' } })).toStrictEqual(false)
})

test('Do not handle chromewebdata origin', () => {
  expect(messageHandler({ 'type': 'message', 'origin': 'chromewebdata', 'data': { 'command': 'ipfs' } })).toStrictEqual(false)
})

test('Do not handle wrong command', () => {
  expect(messageHandler({ 'type': 'message', 'origin': document.location.origin, 'data': { 'command': 'any' } })).toStrictEqual(false)
})

test('Validate no data payload', () => {
  expect(messageHandler({ 'type': 'message', 'origin': document.location.origin })).toStrictEqual(false)
})

test('Validate empty data payload ', () => {
  expect(messageHandler({ 'type': 'message', 'origin': document.location.origin, 'data': { } })).toStrictEqual(false)
})

test('Validate wrong event type ', () => {
  expect(messageHandler({ 'type': 'command', 'origin': document.location.origin, 'data': { 'command': 'ipfs' } })).toStrictEqual(false)
})
