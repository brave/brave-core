/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

/*
  Environment Variables

  USERNAME        - valid Transifex user name with read privileges
  PASSWORD        - password for above username

  LANG [optional] - single language code to retrieve in xx-XX format (I.e. en-US)

	run:  USERNAME=brave.dev PASSWORD=<pw> node downloadLanguages.js
*/

'use strict'

const path = require('path')
const fs = require('fs')
const request = require('request')

// The names of the directories in the locales folder are used as a list of languages to retrieve
var languages = [ "fr", "pl", "ru", "de", "zh", "zh_TW", "id_ID", "it", "ja", "ko_KR", "ms", "pt_BR", "es", "uk", "nb", "sv" ]
// Support retrieving a single language
if (process.env.LANG_CODE) {
  languages = [process.env.LANG_CODE]
}

// Setup the credentials
var username = process.env.USERNAME
var password = process.env.PASSWORD
if (!(username && password)) {
  username = 'brave.dev'
  password = ('' + fs.readFileSync(path.join(process.env.HOME, '.brave-transifex-password'))).trim()
  if (!(username && password)) {
    throw new Error('The USERNAME and PASSWORD environment variables must be set to the Transifex credentials')
  }
}

// URI and resource list
const TEMPLATE = 'https://www.transifex.com/api/2/project/brave-ios/resource/RESOURCE_SLUG/translation/LANG_CODE/?file'

console.log('languages ' + languages + '\n')

// For each language / resource combination
languages.forEach(function (languageCode) {
    // Build the URI
    var URI = TEMPLATE.replace('RESOURCE_SLUG', 'bravexliff')
    URI = URI.replace('LANG_CODE', languageCode)
    // Authorize and request the translation file
    request.get(URI, {
      'auth': {
        'user': username,
        'pass': password,
        'sendImmediately': true
      }
    }, function (error, response, body) {
      if (error) {
        // Report errors (often timeouts)
        console.log(error.toString())
      } else {
        if (response.statusCode === 401) {
          throw new Error('Unauthorized - Are the USERNAME and PASSWORD env vars set correctly?')
        }
        if (Math.floor(response.statusCode/100) != 2) {
          throw new Error("Error http code: " + response.statusCode)
        }

        // Build the filename and store the translation file
        var filename = path.join(__dirname, 'translated-xliffs', languageCode.replace('_', '-') + '.xliff')
        console.log('[*] ' + filename)
        if (process.env.TEST) {
          console.log(body)
        } else {
          fs.writeFileSync(filename, body)
        }
      }
    })
})
