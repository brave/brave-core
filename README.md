# Brave Only Extension

[![Build Status](https://travis-ci.org/brave/brave-extension.svg?branch=master)](https://travis-ci.org/brave/brave-extension)

> Experiment moving part of the Brave UI into an extension for use in Brave only (Not Chrome, it will use APIs not available in Chrome)

## Installation

```bash
# clone brave-extension
$ git clone git@github.com:brave/brave-extension.git

# Install dependencies
$ cd brave-extension
$ npm install
```

## Development

```bash
# Build files will appear in './dev'
# Start webpack development server
$ npm run dev
```

## Release

### Build

```bash
# build files to './build'
$ npm run build
```

### Packaging


```bash
# compress release into a brave.zip and brave.crx.
$ npm run build
$ npm run compress -- [options]
```

## Test

* `test/app`: React components, Redux actions & reducers tests
* `test/e2e`: E2E tests (use [chromedriver](https://www.npmjs.com/package/chromedriver), [selenium-webdriver](https://www.npmjs.com/package/selenium-webdriver))

```bash
# lint
$ npm run test-unit

# test/e2e
$ npm run-script build
$ npm run test-e2e
```

## LICENSE

[MPL-2](LICENSE)
