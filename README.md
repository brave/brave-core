# Brave Chromium Extension

[![Build Status](https://travis-ci.org/brave/brave-extension.svg?branch=master)](https://travis-ci.org/brave/brave-extension)

> Chromium extension providing custom Brave functionality.

## Installation

```bash
# clone it
$ git clone https://github.com/brave/brave-extension.git

# Install dependencies
$ npm install
```

## Development

* Run script
```bash
# Build files will appear in './dev'
# Start webpack development server
$ npm run dev
```

#### Building release

## Build

```bash
# build files to './build'
$ npm run build
```

## Compress

```bash
# compress build folder to {manifest.name}.zip and crx
$ npm run build
$ npm run compress -- [options]
```

## Test

* `test/app`: React components, Redux actions & reducers tests
* `test/e2e`: E2E tests (use [chromedriver](https://www.npmjs.com/package/chromedriver), [selenium-webdriver](https://www.npmjs.com/package/selenium-webdriver))

```bash
# lint
$ npm run lint
# test/app
$ npm test
$ npm test -- --watch  # watch files
# test/e2e
$ npm run build
$ npm run test-e2e
```

## LICENSE

[MPL-2](LICENSE)
