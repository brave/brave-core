# React Chrome Extension Boilerplate

[![Build Status](https://travis-ci.org/jhen0409/react-chrome-extension-boilerplate.svg?branch=master)](https://travis-ci.org/jhen0409/react-chrome-extension-boilerplate)
[![Build status: Windows](https://ci.appveyor.com/api/projects/status/b5xy6ev6oykth0d2/branch/master?svg=true)](https://ci.appveyor.com/project/jhen0409/react-chrome-extension-boilerplate/branch/master)
[![NPM version](http://img.shields.io/npm/v/react-chrome-extension-boilerplate.svg?style=flat)](https://www.npmjs.com/package/react-chrome-extension-boilerplate)
[![Dependency Status](https://david-dm.org/jhen0409/react-chrome-extension-boilerplate.svg)](https://david-dm.org/jhen0409/react-chrome-extension-boilerplate)
[![devDependency Status](https://david-dm.org/jhen0409/react-chrome-extension-boilerplate/dev-status.svg)](https://david-dm.org/jhen0409/react-chrome-extension-boilerplate#info=devDependencies)

> Boilerplate for Chrome Extension React.js project.

## Features

 - Simple [React](https://github.com/facebook/react)/[Redux](https://github.com/rackt/redux) examples of Chrome Extension Window & Popup & Inject pages
 - Hot reloading React/Redux (Using [Webpack](https://github.com/webpack/webpack) and [React Transform](https://github.com/gaearon/react-transform))
 - Write code with ES2015+ syntax (Using [Babel](https://github.com/babel/babel))
 - E2E tests of Window & Popup & Inject pages (Using [Chrome Driver](https://www.npmjs.com/package/chromedriver), [Selenium Webdriver](https://www.npmjs.com/package/selenium-webdriver))

## Examples

The example is edited from [Redux](https://github.com/rackt/redux) TodoMVC example.

#### Popup

![Popup](https://cloud.githubusercontent.com/assets/3001525/14128490/dc05e9f8-f653-11e5-9de6-82d1de01844a.gif)

The `todos` state will be saved to `chrome.storage.local`.

#### Window

![Window](https://cloud.githubusercontent.com/assets/3001525/14128489/da176b62-f653-11e5-9bff-fefc35232358.gif)

The context menu is created by [chrome/extension/background/contextMenus.js](chrome/extension/background/contextMenus.js).

#### Inject page

The inject script is being run by [chrome/extension/background/inject.js](chrome/extension/background/inject.js). A simple example will be inject bottom of page(`https://github.com/*`) if you visit.

If you are receiving the error message `Failed to load resource: net::ERR_INSECURE_RESPONSE`, you need to allow invalid certificates for resources loaded from localhost. You can do this by visiting the following URL: `chrome://flags/#allow-insecure-localhost` and enabling **Allow invalid certificates for resources loaded from localhost**.

## Installation

```bash
# clone it
$ git clone https://github.com/jhen0409/react-chrome-extension-boilerplate.git

# Install dependencies
$ npm install
```

## Development

* Run script
```bash
# build files to './dev'
# start webpack development server
$ npm run dev
```
* If you're developing Inject page, please allow `https://localhost:3000` connections. (Because `injectpage` injected GitHub (https) pages, so webpack server procotol must be https.)
* [Load unpacked extensions](https://developer.chrome.com/extensions/getstarted#unpacked) with `./dev` folder.

#### React/Redux hot reload

This boilerplate uses `Webpack` and `react-transform`, and use `Redux`. You can hot reload by editing related files of Popup & Window & Inject page.

#### Using Redux DevTools Extension

You can use [redux-devtools-extension](https://github.com/zalmoxisus/redux-devtools-extension) on development mode.

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

#### Options

If you want to build `crx` file (auto update), please provide options, and add `update.xml` file url in [manifest.json](https://developer.chrome.com/extensions/autoupdate#update_url manifest.json).

* --app-id: your extension id (can be get it when you first release extension)
* --key: your private key path (default: './key.pem')  
  you can use `npm run compress-keygen` to generate private key `./key.pem`
* --codebase: your `crx` file url

See [autoupdate guide](https://developer.chrome.com/extensions/autoupdate) for more information.

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

[MIT](LICENSE)
