const messageHandler = require('./ipfs_onboarding.js')

test('Validate origin success', () => {
  expect(messageHandler({ 'type': 'message', 'origin': document.location.origin, 'data': { 'command': 'ipfs', 'code': 100, 'value': '' } })).toStrictEqual(true)
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

test('No data payload', () => {
  expect(messageHandler({ 'type': 'message', 'origin': document.location.origin })).toStrictEqual(false)
})

test('Validate empty data payload ', () => {
  expect(messageHandler({ 'type': 'message', 'origin': document.location.origin, 'data': { } })).toStrictEqual(false)
})

test('Wrong event type ', () => {
  expect(messageHandler({ 'type': 'command', 'origin': document.location.origin, 'data': { 'command': 'ipfs' } })).toStrictEqual(false)
})

test('Code and value as numbers', () => {
  expect(messageHandler({ 'type': 'message', 'origin': document.location.origin, 'data': { 'command': 'ipfs', 'code': `100`, 'value': '1' } })).toStrictEqual(true)
})

test('Code and value as string', () => {
  expect(messageHandler({ 'type': 'message', 'origin': document.location.origin, 'data': { 'command': 'ipfs', 'code': `aa100`, 'value': 'a1' } })).toStrictEqual(false)
})

test('No value', () => {
  expect(messageHandler({ 'type': 'message', 'origin': document.location.origin, 'data': { 'command': 'ipfs', 'code': 100 } })).toStrictEqual(false)
})

test('No code', () => {
  expect(messageHandler({ 'type': 'message', 'origin': document.location.origin, 'data': { 'command': 'ipfs', 'value': 100 } })).toStrictEqual(false)
})
