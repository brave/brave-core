const path = require('path')
const fs = require('fs-extra')
const ip = require('ip')
const URL = require('url').URL
const config = require('../lib/config')
const util = require('../lib/util')
const whitelistedUrlPrefixes = require('./whitelistedUrlPrefixes')
const whitelistedUrlPatterns = require('./whitelistedUrlPatterns')
const whitelistedUrlProtocols = [
  'chrome-extension:',
  'chrome:',
  'brave:',
  'file:',
  'data:',
  'blob:'
]

const start = (passthroughArgs, buildConfig = config.defaultBuildConfig, options) => {
  config.buildConfig = buildConfig
  config.update(options)

  let braveArgs = [
    '--enable-logging',
    '--v=' + options.v,
  ]
  if (options.vmodule) {
    braveArgs.push('--vmodule=' + options.vmodule);
  }
  if (options.no_sandbox) {
    braveArgs.push('--no-sandbox')
  }
  if (options.disable_brave_extension) {
    braveArgs.push('--disable-brave-extension')
  }
  if (options.disable_brave_rewards_extension) {
    braveArgs.push('--disable-brave-rewards-extension')
  }
  if (options.disable_pdfjs_extension) {
    braveArgs.push('--disable-pdfjs-extension')
  }
  if (options.disable_webtorrent_extension) {
    braveArgs.push('--disable-webtorrent-extension')
  }
  if (options.ui_mode) {
    braveArgs.push(`--ui-mode=${options.ui_mode}`)
  }
  if (!options.enable_brave_update) {
    // This only has meaning with MacOS and official build.
    braveArgs.push('--disable-brave-update')
  }
  if (options.disable_doh) {
    braveArgs.push('--disable-doh')
  }
  if (options.single_process) {
    braveArgs.push('--single-process')
  }
  if (options.show_component_extensions) {
    braveArgs.push('--show-component-extension-options')
  }
  if (options.rewards) {
    braveArgs.push(`--rewards=${options.rewards}`)
  }
  if (options.brave_ads_testing) {
    braveArgs.push('--brave-ads-testing')
  }
  if (options.brave_ads_debug) {
    braveArgs.push('--brave-ads-debug')
  }
  if (options.brave_ads_production) {
    braveArgs.push('--brave-ads-production')
  }
  if (options.brave_ads_staging) {
    braveArgs.push('--brave-ads-staging')
  }
  braveArgs = braveArgs.concat(passthroughArgs)

  let user_data_dir
  if (options.user_data_dir_name) {
    if (process.platform === 'darwin') {
      user_data_dir = path.join(process.env.HOME, 'Library', 'Application\\ Support', 'BraveSoftware', options.user_data_dir_name)
    } else if (process.platform === 'win32') {
      user_data_dir = path.join(process.env.LocalAppData, 'BraveSoftware', options.user_data_dir_name)
    } else {
      user_data_dir = path.join(process.env.HOME, '.config', 'BraveSoftware', options.user_data_dir_name)
    }
    braveArgs.push('--user-data-dir=' + user_data_dir);
  }
  const networkLogFile = path.resolve(path.join(config.rootDir, 'network_log.json'))
  if (options.network_log) {
    braveArgs.push(`--log-net-log=${networkLogFile}`)
    braveArgs.push(`--net-log-capture-mode=Everything`)
    if (user_data_dir) {
      // clear the data directory before doing a network test
      fs.removeSync(user_data_dir.replace('\\', ''))
      if (fs.existsSync(networkLogFile)) {
        fs.unlinkSync(networkLogFile)
      }
      if (fs.existsSync('network-audit-results.json')) {
        fs.unlinkSync('network-audit-results.json')
      }
    }
  }

  let cmdOptions = {
    stdio: 'inherit',
    timeout: options.network_log ? 120000 : undefined,
    continueOnFail: options.network_log ? true : false,
    shell: process.platform === 'darwin' ? true : false,
    killSignal: options.network_log && process.env.RELEASE_TYPE ? 'SIGKILL' : 'SIGTERM'
  }

  if (options.network_log) {
    console.log('Network audit started. Logging requests for the next 2min or until you quit Brave...')
  }

  let outputPath = options.output_path
  if (!outputPath) {
    outputPath = path.join(config.outputDir, 'brave')
    if (process.platform === 'win32') {
      outputPath = outputPath + '.exe'
    } else if (process.platform === 'darwin') {
      outputPath = fs.readFileSync(outputPath + '_helper').toString().trim()
    }
  }
  util.run(outputPath, braveArgs, cmdOptions)

  if (options.network_log) {
    let exitCode = 0
    let jsonOutput = {}
    // Read the network log
    let jsonContent = fs.readFileSync(networkLogFile, 'utf8').trim()
    // On windows netlog ends abruptly causing JSON parsing errors
    if (!jsonContent.endsWith('}]}')) {
      const n = jsonContent.lastIndexOf('},')
      jsonContent = jsonContent.substring(0, n) + '}]}'
    }
    jsonOutput = JSON.parse(jsonContent)

    const URL_REQUEST_TYPE = jsonOutput.constants.logSourceType.URL_REQUEST
    const URL_REQUEST_FAKE_RESPONSE_HEADERS_CREATED = jsonOutput.constants.logEventTypes.URL_REQUEST_FAKE_RESPONSE_HEADERS_CREATED
    const urlRequests = jsonOutput.events.filter((event) => {
      if (event.type === URL_REQUEST_FAKE_RESPONSE_HEADERS_CREATED) {
        // showing these helps determine which URL requests which don't
        // actually hit the network
        return true
      }
      if (event.source.type === URL_REQUEST_TYPE) {
        if (!event.params) {
          return false
        }
        const url = event.params.url
        if (!url) {
          return false
        }
        const urlParsed = new URL(url)
        const hostname = urlParsed.hostname
        if (/^[a-z]+$/.test(hostname)) {
          // Chromium sometimes sends requests to random non-resolvable hosts
          return false
        }
        if (whitelistedUrlProtocols.includes(urlParsed.protocol)) {
          return false
        }
        const foundPrefix = whitelistedUrlPrefixes.find((prefix) => {
          return url.startsWith(prefix)
        })
        const foundPattern = whitelistedUrlPatterns.find((pattern) => {
          return RegExp('^' + pattern).test(url)
        })
        if (!foundPrefix && !foundPattern) {
          // Check if the URL is a private IP
          try {
            if (ip.isPrivate(hostname)) {
              // Warn but don't fail the audit
              console.log('NETWORK AUDIT WARN:', url)
              return true
            }
          } catch (e) {}
          // This is not a whitelisted URL! log it and exit with non-zero
          console.log('NETWORK AUDIT FAIL:', url)
          exitCode = 1
        }
        return true
      }
      return false
    })
    fs.writeJsonSync('network-audit-results.json', urlRequests)
    if (exitCode > 0) {
      console.log(`network-audit failed. import ${networkLogFile} in chrome://net-internals for more details.`)
    } else {
      console.log('network audit passed.')
    }
    process.exit(exitCode)
  }
}

module.exports = start
