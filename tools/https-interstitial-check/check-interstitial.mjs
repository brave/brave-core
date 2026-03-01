#!/usr/bin/env node
// Launches Brave and checks whether navigating to a URL triggers
// an HTTPS-upgrade security interstitial (warning dialog or blocking page).
//
// Detection strategy (desktop Brave uses a native dialog, not page HTML):
//   1. Navigation error: ERR_BLOCKED_BY_CLIENT signals the throttle blocked HTTP.
//   2. DOM probe: Old interstitial UI injects identifiable elements.
//   3. Blank-body heuristic: Dialog UI loads an empty error page.
//   4. CDP Page.interstitialShown event (fires for legacy interstitials).
//   5. Screenshot: Captures visual state for manual verification.

import { chromium } from 'playwright-core';
import { existsSync, mkdirSync, writeFileSync } from 'node:fs';
import { mkdtempSync } from 'node:fs';
import { execSync, spawn } from 'node:child_process';
import { tmpdir } from 'node:os';
import { parseArgs } from 'node:util';
import { join, resolve } from 'node:path';

// ---------------------------------------------------------------------------
// CLI
// ---------------------------------------------------------------------------

const { values: opts } = parseArgs({
  options: {
    url:             { type: 'string',  short: 'u' },
    'brave-path':    { type: 'string',  short: 'b' },
    'user-data-dir': { type: 'string',  short: 'd' },
    strict:          { type: 'boolean', default: false },
    headless:        { type: 'boolean', default: false },
    timeout:         { type: 'string',  default: '30000' },
    screenshot:      { type: 'string',  short: 's', default: '' },
    'window-shot':   { type: 'string',  short: 'w', default: '' },
    json:            { type: 'boolean', default: false },
    help:            { type: 'boolean', short: 'h', default: false },
  },
  strict: false,
});

if (opts.help || !opts.url) {
  console.log(`Usage: node check-interstitial.mjs -u <url> [options]

Options:
  -u, --url <url>            URL to navigate to (required)
  -b, --brave-path <path>    Path to Brave binary
  -d, --user-data-dir <dir>  Use a specific user data directory (profile)
      --strict                Create a temp profile with Strict HTTPS upgrade mode
      --headless              Run headless (default: false)
      --timeout <ms>          Navigation timeout in ms (default: 30000)
  -s, --screenshot <path>    Save a screenshot to this path
      --json                  Output results as JSON
  -h, --help                 Show this help message
`);
  process.exit(opts.help ? 0 : 1);
}

const targetUrl = opts.url;
const headless = opts.headless;
const timeout = parseInt(opts.timeout, 10);
const screenshotPath = opts.screenshot || '';

// ---------------------------------------------------------------------------
// Locate Brave binary
// ---------------------------------------------------------------------------

function findBrave() {
  if (opts['brave-path']) {
    const p = resolve(opts['brave-path']);
    if (!existsSync(p)) {
      console.error(`Brave binary not found at: ${p}`);
      process.exit(1);
    }
    return p;
  }

  // Script lives at src/brave/tools/https-interstitial-check/
  // Build output lives at src/out/<config>/
  const srcDir = resolve(import.meta.dirname, '../../..');
  const candidates = [
    // Local builds (arm64 variants first, then generic)
    resolve(srcDir, 'out/Component_arm64/Brave Browser Development.app/Contents/MacOS/Brave Browser Development'),
    resolve(srcDir, 'out/Debug_arm64/Brave Browser Development.app/Contents/MacOS/Brave Browser Development'),
    resolve(srcDir, 'out/Component/Brave Browser Development.app/Contents/MacOS/Brave Browser Development'),
    resolve(srcDir, 'out/Debug/Brave Browser Development.app/Contents/MacOS/Brave Browser Development'),
    resolve(srcDir, 'out/Release/Brave Browser Development.app/Contents/MacOS/Brave Browser Development'),
    // Installed channels
    '/Applications/Brave Browser Nightly.app/Contents/MacOS/Brave Browser Nightly',
    '/Applications/Brave Browser Beta.app/Contents/MacOS/Brave Browser Beta',
    '/Applications/Brave Browser.app/Contents/MacOS/Brave Browser',
  ];

  for (const c of candidates) {
    if (existsSync(c)) return c;
  }

  console.error('Could not find a Brave binary. Use --brave-path to specify one.');
  process.exit(1);
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

async function checkInterstitial(url) {
  const bravePath = findBrave();
  const log = opts.json ? () => {} : console.log;

  log(`Brave binary : ${bravePath}`);
  log(`Target URL   : ${url}`);
  log(`Headless     : ${headless}`);
  log(`Timeout      : ${timeout} ms`);
  log('');

  // -- Profile setup --------------------------------------------------------

  let userDataDir = opts['user-data-dir'] ? resolve(opts['user-data-dir']) : null;

  const isBrave = bravePath.toLowerCase().includes('brave');

  if (opts.strict && !userDataDir) {
    // Create a temp profile pre-seeded with strict HTTPS mode.
    userDataDir = mkdtempSync(join(tmpdir(), 'https-interstitial-'));
    const profileDir = join(userDataDir, 'Default');
    mkdirSync(profileDir, { recursive: true });

    const prefs = {
      // Suppress first-run UI
      browser: { has_seen_welcome_page: true },
    };

    if (isBrave) {
      // Brave: content setting "httpsUpgrades"
      // CONTENT_SETTING_BLOCK (2) = Strict mode
      // Pattern "*,*" = global (all sites)
      prefs.profile = {
        content_settings: {
          exceptions: {
            httpsUpgrades: {
              '*,*': { setting: 2 },
            },
          },
        },
      };
    } else {
      // Chrome: pref "https_only_mode_enabled" = true enables HTTPS-Only Mode
      prefs.https_only_mode_enabled = true;
    }

    writeFileSync(join(profileDir, 'Preferences'), JSON.stringify(prefs));
    log(`Strict mode  : enabled (temp profile at ${userDataDir})`);
    log(`Browser type : ${isBrave ? 'Brave' : 'Chrome/Chromium'}`);
  }

  const launchArgs = [
    '--no-first-run',
    '--disable-extensions',
    '--disable-component-update',
    '--disable-background-networking',
  ];

  // Only use --silent-launch for Playwright-managed launches (no userDataDir).
  // For manual spawn we want a visible window.
  if (!userDataDir) {
    launchArgs.push('--silent-launch');
  }

  let browser, context, page;
  let childProcess = null;

  if (userDataDir) {
    // Playwright's launchPersistentContext has issues with the Dev build,
    // so we spawn Brave manually and connect via CDP over a debug port.
    const debugPort = 9222 + Math.floor(Math.random() * 1000);
    const allArgs = [
      ...launchArgs,
      `--user-data-dir=${userDataDir}`,
      `--remote-debugging-port=${debugPort}`,
      '--window-size=1280,900',
      '--window-position=100,100',
      headless ? '--headless=new' : '',
      url,
    ].filter(Boolean);

    log(`Debug port   : ${debugPort}`);
    childProcess = spawn(bravePath, allArgs, {
      stdio: ['ignore', 'ignore', 'ignore'],
      detached: false,
    });

    // Wait for the debug port to become available.
    const cdpUrl = `http://127.0.0.1:${debugPort}`;
    for (let i = 0; i < 30; i++) {
      try {
        const resp = await fetch(`${cdpUrl}/json/version`);
        if (resp.ok) break;
      } catch { /* not ready yet */ }
      await new Promise((r) => setTimeout(r, 1000));
    }

    browser = await chromium.connectOverCDP(cdpUrl);
    context = browser.contexts()[0];
    page = context?.pages()[0] || await (context ?? await browser.newContext()).newPage();
    if (!context) context = page.context();
  } else {
    browser = await chromium.launch({
      executablePath: bravePath,
      headless,
      args: launchArgs,
    });
    context = await browser.newContext({
      ignoreHTTPSErrors: false,
    });
    page = await context.newPage();
  }

  // -- CDP listeners -------------------------------------------------------

  const cdp = await context.newCDPSession(page);

  // Security state tracking
  await cdp.send('Security.enable');
  const securityEvents = [];
  cdp.on('Security.securityStateChanged', (evt) => {
    securityEvents.push({
      state: evt.securityState,
      summary: evt.summary,
    });
  });

  // Legacy interstitial event tracking
  let cdpInterstitialShown = false;
  cdp.on('Page.interstitialShown', () => { cdpInterstitialShown = true; });

  // Network error tracking
  const networkErrors = [];
  await cdp.send('Network.enable');
  cdp.on('Network.loadingFailed', (evt) => {
    networkErrors.push({
      url: evt.url,
      errorText: evt.errorText,
      blockedReason: evt.blockedReason,
    });
  });

  // -- Navigate ------------------------------------------------------------

  let navigationError = null;
  let response = null;

  try {
    response = await page.goto(url, {
      waitUntil: 'domcontentloaded',
      timeout,
    });
  } catch (err) {
    navigationError = err;
  }

  // Give the dialog/interstitial time to render
  await page.waitForTimeout(2000);

  // Ensure the window is visible and in front for window screenshots.
  try {
    await page.bringToFront();
    // Also try CDP to set window bounds (ensures it's not minimized).
    const { windowId } = await cdp.send('Browser.getWindowForTarget');
    await cdp.send('Browser.setWindowBounds', {
      windowId,
      bounds: { left: 100, top: 100, width: 1280, height: 900, windowState: 'normal' },
    });
  } catch { /* best effort */ }
  await page.waitForTimeout(500);

  // -- Window screenshot (macOS screencapture, captures native dialogs) ------

  if (opts['window-shot']) {
    const wsPath = resolve(opts['window-shot']);
    try {
      // Bring the browser window to front before capturing.
      // Bring browser window to front and un-minimize it.
      const appName = isBrave ? 'Brave Browser Development' : 'Google Chrome';
      try {
        execSync(`osascript -e '
          tell application "System Events"
            set frontmost of every process whose name contains "${appName}" to true
            tell process "${appName}"
              try
                click menu item "Bring All to Front" of menu "Window" of menu bar 1
              end try
            end tell
          end tell'`, { timeout: 3000 });
      } catch { /* best effort */ }
      await page.waitForTimeout(1000);
      // -x = no sound, -R = region matching window position/size
      execSync(`screencapture -x -R100,100,1280,900 "${wsPath}"`, { timeout: 5000 });
      log(`Window shot  : ${wsPath}`);
    } catch {
      log('Window shot  : screencapture failed (may need screen recording permission)');
    }
  }

  // -- Detect interstitial --------------------------------------------------

  const finalUrl = page.url();

  // Probe for old full-page interstitial DOM elements
  const domProbe = await page.evaluate(() => {
    const qs = (sel) => !!document.querySelector(sel);
    const bodyText = (document.body?.innerText || '').trim();
    return {
      hasMainContent:   qs('#main-content'),
      hasPrimaryButton: qs('#primary-button'),
      hasProceedButton: qs('#proceed-button'),
      hasDetailsButton: qs('#details-button'),
      bodyLength:       bodyText.length,
      bodySnippet:      bodyText.substring(0, 300),
      title:            document.title,
    };
  });

  // Heuristics
  const navErrorText = navigationError?.message || '';
  const isBlockedByClient = navErrorText.includes('ERR_BLOCKED_BY_CLIENT')
    || networkErrors.some((e) => e.errorText === 'net::ERR_BLOCKED_BY_CLIENT');
  const hasOldInterstitial = domProbe.hasMainContent && domProbe.hasPrimaryButton;
  const hasBlankBody = domProbe.bodyLength === 0;
  const isChromeError = finalUrl.startsWith('chrome-error://');

  const isInterstitial =
    hasOldInterstitial              // legacy full-page interstitial
    || cdpInterstitialShown         // CDP event (legacy)
    || (isBlockedByClient && hasBlankBody);  // dialog UI: blocked + blank page

  // -- Screenshot -----------------------------------------------------------

  if (screenshotPath) {
    await page.screenshot({ path: resolve(screenshotPath), fullPage: true });
    log(`Screenshot saved to: ${resolve(screenshotPath)}`);
  }

  // -- Results --------------------------------------------------------------

  const result = {
    url,
    finalUrl,
    interstitialDetected: isInterstitial,
    signals: {
      navigationFailed:   !!navigationError,
      isBlockedByClient,
      hasOldInterstitial,
      cdpInterstitialShown,
      hasBlankBody,
      isChromeError,
      httpStatus:         response?.status() ?? null,
    },
    domProbe,
    securityEvents,
    networkErrors,
  };

  await browser.close();
  if (childProcess) {
    childProcess.kill();
  }
  return result;
}

// ---------------------------------------------------------------------------
// Run
// ---------------------------------------------------------------------------

try {
  const result = await checkInterstitial(targetUrl);

  if (opts.json) {
    console.log(JSON.stringify(result, null, 2));
  } else {
    console.log('--- Results ---');
    console.log(`Final URL            : ${result.finalUrl}`);
    console.log(`HTTP status          : ${result.signals.httpStatus ?? '(none)'}`);
    console.log(`Navigation failed    : ${result.signals.navigationFailed}`);
    console.log(`ERR_BLOCKED_BY_CLIENT: ${result.signals.isBlockedByClient}`);
    console.log(`Old interstitial DOM : ${result.signals.hasOldInterstitial}`);
    console.log(`CDP interstitialShown: ${result.signals.cdpInterstitialShown}`);
    console.log(`Blank body (dialog)  : ${result.signals.hasBlankBody}`);
    console.log(`chrome-error:// URL  : ${result.signals.isChromeError}`);
    console.log(`Security events      : ${result.securityEvents.length}`);
    console.log(`Network errors       : ${result.networkErrors.length}`);
    if (result.domProbe?.bodySnippet) {
      console.log(`Body snippet         : ${result.domProbe.bodySnippet.substring(0, 80)}`);
    }
    console.log('');

    if (result.interstitialDetected) {
      console.log('RESULT: HTTPS upgrade interstitial/dialog DETECTED');
      process.exit(1);
    } else {
      console.log('RESULT: No interstitial detected');
      process.exit(0);
    }
  }

  // JSON mode: exit code based on detection
  if (opts.json) {
    process.exit(result.interstitialDetected ? 1 : 0);
  }
} catch (err) {
  console.error('Fatal error:', err.message);
  process.exit(2);
}
