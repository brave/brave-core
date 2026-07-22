#!/usr/bin/env python3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""
Fires up an Android emulator using the android_33_google_atd_x64 AVD config
(via Chromium's avd.py tooling), installs the latest Brave APK for the
selected channel (nightly/beta/release), and optionally completes the
onboarding flow, saves/loads app data, or verifies background audio playback.

Usage:
    ./brave_emulator_setup.py [OPTIONS]

Run with --help for the full option list.
"""

import argparse
import json
import os
import re
import subprocess
import sys
import tempfile
import time
import xml.etree.ElementTree as ET

from datetime import datetime
from typing import List, Optional, Sequence, Tuple

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
SRC_DIR = os.path.normpath(os.path.join(SCRIPT_DIR, '..', '..'))
AVD_PY = os.path.join(SRC_DIR, 'tools', 'android', 'avd', 'avd.py')
AVD_CONFIG = os.path.join(SRC_DIR, 'tools', 'android', 'avd', 'proto',
                          'android_33_google_atd_x64.textpb')
DEPOT_TOOLS = os.path.join(SRC_DIR, 'third_party', 'depot_tools')
VPYTHON3 = os.path.join(DEPOT_TOOLS, 'vpython3')

ANDROID_SDK = os.environ.get('ANDROID_HOME',
                             os.path.expanduser('~/Library/Android/sdk'))
ADB_PATH = os.path.join(ANDROID_SDK, 'platform-tools', 'adb')
EMULATOR_PATH = os.path.join(ANDROID_SDK, 'emulator', 'emulator')

AVD_NAME = 'android_33_google_atd_x64'
# CIPD-installed AVD home (contains the AVD config and system images).
CIPD_ROOT = os.path.join(SRC_DIR, '.android_emulator', AVD_NAME)
BRAVE_ACTIVITY = 'com.google.android.apps.chrome.Main'
SERIAL = 'emulator-5554'

DEVICE_BACKUP_PATH = '/sdcard/brave_app_data.tar.gz'
DEVICE_UI_DUMP_PATH = '/sdcard/ui_dump.xml'

CHANNELS = {
    'nightly': {
        'package': 'com.brave.browser_nightly',
        'label': 'Nightly',
        'release_filter': 'Nightly',
    },
    'beta': {
        'package': 'com.brave.browser_beta',
        'label': 'Beta',
        'release_filter': 'Beta',
    },
    'release': {
        'package': 'com.brave.browser',
        'label': 'Release',
        'release_filter': 'Release',
    },
}

KEYCODE_HOME = 3
KEYCODE_BACK = 4
KEYCODE_MENU = 82

# ---------------------------------------------------------------------------
# Timings
#
# These values were tuned empirically against a live emulator. The audio
# constants in particular depend on the AudioFlinger mixer standby delay (3s)
# and on how long Brave takes to tear down playback when backgrounded, so
# lowering them reintroduces false positives. Change them only alongside a
# fresh round of on-device validation.
# ---------------------------------------------------------------------------
BOOT_TIMEOUT = 180
BOOT_POLL_INTERVAL = 3
UI_POLL_INTERVAL = 2
DEFAULT_TAP_TIMEOUT = 30

# Audio is considered active when the primary output thread is out of standby
# and wrote a buffer within this window.
AUDIO_LAST_WRITE_MAX_MS = 1000
# Foreground confirmation: any one of these probes seeing audio is enough.
AUDIO_FOREGROUND_ATTEMPTS = 5
AUDIO_FOREGROUND_INTERVAL = 2
# Settle time after pressing HOME: covers the 3s mixer standby delay plus the
# time the app needs to actually stop playback if it is going to.
AUDIO_BACKGROUND_SETTLE = 8
# Background verification: ALL probes must see audio to PASS, which rules out
# false positives from lingering mixer buffers.
AUDIO_BACKGROUND_ATTEMPTS = 5
AUDIO_BACKGROUND_INTERVAL = 5


class SetupError(Exception):
    """Fatal error; reported and turned into a non-zero exit code."""


def log(message: str = '') -> None:
    print(f"[{datetime.now().strftime('%H:%M:%S')}] {message}", flush=True)


def human_size(num_bytes: int) -> str:
    size = float(num_bytes)
    for unit in ('B', 'K', 'M', 'G'):
        if size < 1024 or unit == 'G':
            return f'{size:.0f}{unit}' if unit == 'B' else f'{size:.1f}{unit}'
        size /= 1024
    return f'{size:.1f}G'


def wait_until(predicate, timeout: float, interval: float) -> bool:
    """Poll `predicate` until it returns True or `timeout` seconds elapse."""
    deadline = time.monotonic() + timeout
    while True:
        if predicate():
            return True
        if time.monotonic() >= deadline:
            return False
        time.sleep(interval)


# ---------------------------------------------------------------------------
# adb wrapper
# ---------------------------------------------------------------------------
class Adb:
    """Thin wrapper around `adb -s <serial>`."""

    def __init__(self, adb_path: str, serial: str = SERIAL):
        self.adb_path = adb_path
        self.serial = serial

    def _run(self,
             args: Sequence[str],
             targeted: bool = True,
             check: bool = False,
             timeout: Optional[float] = None) -> subprocess.CompletedProcess:
        cmd = [self.adb_path]
        if targeted:
            cmd += ['-s', self.serial]
        cmd += list(args)
        return subprocess.run(cmd,
                              check=check,
                              timeout=timeout,
                              stdout=subprocess.PIPE,
                              stderr=subprocess.PIPE,
                              text=True,
                              errors='replace')

    def run(self, *args: str, **kwargs) -> subprocess.CompletedProcess:
        return self._run(args, **kwargs)

    def shell(self, *args: str, **kwargs) -> subprocess.CompletedProcess:
        return self._run(('shell', ) + args, **kwargs)

    def out(self, *args: str) -> str:
        """Stdout of an `adb shell` command, or '' on failure."""
        result = self._run(('shell', ) + args)
        return result.stdout if result.returncode == 0 else ''

    def ok(self, *args: str) -> bool:
        """True when an `adb shell` command exits zero."""
        return self._run(('shell', ) + args).returncode == 0

    def check(self, *args: str, error: str) -> None:
        """Run an `adb shell` command, raising SetupError on failure."""
        result = self._run(('shell', ) + args)
        if result.returncode != 0:
            raise SetupError(f'{error}\n{result.stderr.strip()}')

    def keyevent(self, code: int) -> None:
        self.shell('input', 'keyevent', str(code))

    def tap(self, x: int, y: int) -> None:
        self.shell('input', 'tap', str(x), str(y))

    def is_connected(self) -> bool:
        return self._run(('get-state', )).returncode == 0

    def is_app_running(self, package: str) -> bool:
        return self.ok('pidof', package)

    def wait_for_device(self) -> None:
        log('Waiting for device to come online...')
        self._run(('wait-for-device', ), targeted=False)
        time.sleep(2)

    def wait_for_boot(self) -> None:
        log(f'Waiting for emulator to boot (timeout: {BOOT_TIMEOUT}s)...')

        def booted() -> bool:
            return self.out('getprop', 'sys.boot_completed').strip() == '1'

        if not wait_until(booted, BOOT_TIMEOUT, BOOT_POLL_INTERVAL):
            raise SetupError(
                f'Emulator failed to boot within {BOOT_TIMEOUT}s.')
        log('Emulator booted successfully.')

    def launch_app(self, package: str, url: Optional[str] = None) -> None:
        component = f'{package}/{BRAVE_ACTIVITY}'
        if url:
            self.shell('am', 'start', '-n', component, '-a',
                       'android.intent.action.VIEW', '-d', url)
        else:
            self.shell('am', 'start', '-n', component, '-a',
                       'android.intent.action.MAIN', '-c',
                       'android.intent.category.LAUNCHER')


# ---------------------------------------------------------------------------
# UI automation
# ---------------------------------------------------------------------------
BOUNDS_RE = re.compile(r'\[(\d+),(\d+)\]\[(\d+),(\d+)\]')


def _bounds_center(bounds: str) -> Optional[Tuple[int, int]]:
    match = BOUNDS_RE.match(bounds or '')
    if not match:
        return None
    x1, y1, x2, y2 = (int(g) for g in match.groups())
    return (x1 + x2) // 2, (y1 + y2) // 2


def find_center(xml: str, attr: str, value: str) -> Optional[Tuple[int, int]]:
    """Center coordinates of the first node whose `attr` ends with `value`.

    uiautomator resource ids are fully qualified (`pkg:id/name`), so callers
    pass the trailing segment and matching is a suffix test.
    """
    if not xml:
        return None
    try:
        root = ET.fromstring(xml)
    except ET.ParseError:
        # A truncated dump still often contains the node we want, so fall back
        # to scanning the raw text.
        pattern = (re.escape(attr) + r'="[^"]*' + re.escape(value) +
                   r'"[^>]*bounds="(\[\d+,\d+\]\[\d+,\d+\])"')
        match = re.search(pattern, xml)
        return _bounds_center(match.group(1)) if match else None

    for node in root.iter('node'):
        if node.get(attr, '').endswith(value):
            center = _bounds_center(node.get('bounds', ''))
            if center:
                return center
    return None


class Ui:
    """uiautomator-driven view of the device screen."""

    def __init__(self, adb: Adb):
        self.adb = adb

    def dump(self) -> str:
        self.adb.shell('uiautomator', 'dump', DEVICE_UI_DUMP_PATH)
        return self.adb.out('cat', DEVICE_UI_DUMP_PATH)

    def _tap_by(self, attr: str, value: str, timeout: int) -> bool:
        deadline = time.monotonic() + timeout
        while True:
            center = find_center(self.dump(), attr, value)
            if center:
                label = value if attr == 'resource-id' else f"text '{value}'"
                log(f'Tapping {label} at ({center[0]}, {center[1]})')
                self.adb.tap(*center)
                time.sleep(2)
                return True
            if time.monotonic() >= deadline:
                kind = 'Element' if attr == 'resource-id' else 'Text'
                log(f"WARNING: {kind} '{value}' not found within "
                    f'{timeout}s - skipping.')
                return False
            time.sleep(UI_POLL_INTERVAL)

    def tap_by_id(self,
                  id_suffix: str,
                  timeout: int = DEFAULT_TAP_TIMEOUT) -> bool:
        return self._tap_by('resource-id', id_suffix, timeout)

    def tap_by_text(self,
                    text: str,
                    timeout: int = DEFAULT_TAP_TIMEOUT) -> bool:
        return self._tap_by('text', text, timeout)

    def is_visible(self, id_suffix: str) -> bool:
        return re.search(f'resource-id="[^"]*{re.escape(id_suffix)}"',
                         self.dump()) is not None


# ---------------------------------------------------------------------------
# Onboarding flow
# ---------------------------------------------------------------------------
VARIANT_Y_MARKERS = ('onboarding_sure', 'onboarding_later')
VARIANT_X_MARKERS = ('btn_positive', 'btn_negative')
FALLBACK_BUTTON_IDS = ('btn_positive', 'btn_negative', 'onboarding_sure',
                       'onboarding_later', 'onboarding_continue',
                       'onboarding_start_browsing')
BROWSER_CHROME_MARKERS = ('toolbar', 'url_bar', 'search_box', 'tab_switcher',
                          'home_button')


def _contains_any(text: str, markers: Sequence[str]) -> bool:
    return any(marker in text for marker in markers)


class Onboarding:
    """Drives Brave's first-run onboarding to completion."""

    def __init__(self, adb: Adb, ui: Ui):
        self.adb = adb
        self.ui = ui

    def run(self) -> None:
        dump = self.ui.dump()

        # Handle any system "Set as default browser" dialog.
        if 'android:id/button1' in dump:
            log('Detected system default browser dialog - dismissing...')
            self.ui.tap_by_id('android:id/button1', 5)
            time.sleep(2)
            dump = self.ui.dump()

        if _contains_any(dump, VARIANT_Y_MARKERS):
            log('Detected Variant Y onboarding')
            self._variant_y()
            return
        if _contains_any(dump, VARIANT_X_MARKERS):
            log('Detected Variant X (default) onboarding')
            self._variant_x()
            return

        # Brave might still be loading or the splash animation is playing.
        log('Onboarding not yet visible. Waiting for splash animation...')
        time.sleep(10)
        dump = self.ui.dump()

        if _contains_any(dump, VARIANT_Y_MARKERS):
            log('Detected Variant Y onboarding (after splash)')
            self._variant_y()
        elif _contains_any(dump, VARIANT_X_MARKERS):
            log('Detected Variant X (default) onboarding (after wait)')
            self._variant_x()
        else:
            log('WARNING: Could not detect onboarding variant. '
                'Dumping UI for debug:')
            for line in dump.splitlines()[:50]:
                print(line)
            log('Attempting generic fallback...')
            self._generic_fallback()

    def _variant_x(self) -> None:
        # Step 0: "Set as Default Browser" - tap the positive button.
        log('[Variant X] Step 0: Set as Default Browser')
        if self.ui.is_visible('btn_positive'):
            self.ui.tap_by_id('btn_positive', 10)
            time.sleep(3)

        # Handle the OS-level default browser role request if it appears.
        dump = self.ui.dump()
        if re.search(r'default.*browser|role.*request|android\.app\.role',
                     dump, re.IGNORECASE):
            log('Dismissing OS default browser dialog...')
            self.adb.keyevent(KEYCODE_BACK)
            time.sleep(2)

        # Step 1: WDP (Web Discovery Project) - skip with "Maybe Later".
        log('[Variant X] Step 1: WDP page')
        if self.ui.is_visible('btn_negative'):
            self.ui.tap_by_id('btn_negative', 10)
            time.sleep(3)

        # Step 2: Analytics consent - accept with "Continue".
        log('[Variant X] Step 2: Analytics consent')
        if self.ui.is_visible('btn_positive'):
            self.ui.tap_by_id('btn_positive', 10)
            time.sleep(3)

        log('[Variant X] Onboarding completed.')

    def _variant_y(self) -> None:
        # Page 0: Help Brave Search - tap "Maybe Later" (skip WDP).
        log('[Variant Y] Page 0: Help Brave Search')
        if not self.ui.tap_by_id('onboarding_later', 15):
            self.ui.tap_by_id('onboarding_sure', 5)
        time.sleep(3)

        # Page 1: Block Interruptions - tap "Continue".
        log('[Variant Y] Page 1: Block Interruptions')
        self.ui.tap_by_id('onboarding_continue', 15)
        time.sleep(3)

        # Page 2: Make Brave Better - tap "Start Browsing".
        log('[Variant Y] Page 2: Make Brave Better')
        self.ui.tap_by_id('onboarding_start_browsing', 15)
        time.sleep(3)

        log('[Variant Y] Onboarding completed.')

    def _generic_fallback(self) -> None:
        log('Attempting fallback onboarding completion...')

        for _ in range(5):
            dump = self.ui.dump()

            for button_id in FALLBACK_BUTTON_IDS:
                if button_id in dump:
                    self.ui.tap_by_id(button_id, 5)
                    time.sleep(3)
                    break

            # Check whether we have passed onboarding by looking for browser
            # chrome elements.
            if _contains_any(self.ui.dump(), BROWSER_CHROME_MARKERS):
                log('Browser UI detected - onboarding appears complete.')
                return

            # Try pressing back as a last resort.
            self.adb.keyevent(KEYCODE_BACK)
            time.sleep(2)

    def dismiss_notification_permission(self) -> None:
        dump = self.ui.dump()
        if not re.search(r'allow|notification', dump, re.IGNORECASE):
            return
        if not _contains_any(
                dump,
            ('permission_allow_button', 'com.android.permissioncontroller')):
            return
        log('Dismissing notification permission dialog...')
        if not self.ui.tap_by_id('permission_allow_button', 5):
            if not self.ui.tap_by_text('Allow', 5):
                self.ui.tap_by_text("Don't allow", 5)
        time.sleep(2)


# ---------------------------------------------------------------------------
# Audio playback inspection
# ---------------------------------------------------------------------------
def is_audio_playing(dumpsys_output: str) -> bool:
    """True when the primary output mixer is actively writing audio.

    While audio plays, the primary output thread has `Standby: no` and a
    recent `Last write occurred`. When playback pauses or stops, the thread
    enters standby and the timestamp goes stale.

    Kept free of I/O so it can be unit tested against captured dumps.
    """
    for block in re.split(r'(?=Output thread)', dumpsys_output):
        if 'AUDIO_DEVICE_OUT_SPEAKER' not in block:
            continue
        standby = re.search(r'Standby:\s*(yes|no)', block)
        last_write = re.search(r'Last write occurred \(msecs\):\s*(\d+)',
                               block)
        if (standby and standby.group(1) == 'no' and last_write
                and int(last_write.group(1)) < AUDIO_LAST_WRITE_MAX_MS):
            return True
    return False


class PlaybackCheck:
    """Verifies that audio keeps playing once Brave is backgrounded."""

    def __init__(self, adb: Adb, ui: Ui, package: str, label: str):
        self.adb = adb
        self.ui = ui
        self.package = package
        self.label = label

    def _audio_active(self) -> bool:
        return is_audio_playing(self.adb.out('dumpsys', 'media.audio_flinger'))

    def _dump_audio_debug(self) -> None:
        log('DEBUG: audio_flinger primary output:')
        dump = self.adb.out('dumpsys', 'media.audio_flinger')
        lines = [
            line for line in dump.splitlines() if re.search(
                r'Standby:|Last write occurred|'
                r'AUDIO_DEVICE_OUT_SPEAKER', line)
        ]
        print('\n'.join(lines) if lines else '(none)')

    def _wait_for_youtube(self) -> bool:
        log('Waiting for YouTube video to load...')
        for attempt in range(1, 16):
            if re.search(r'Subscribe|views|youtube', self.ui.dump(),
                         re.IGNORECASE):
                log(f'YouTube page loaded (attempt {attempt}).')
                return True
            time.sleep(2)
        return False

    def run(self, url: str) -> bool:
        log('=== Step 6c: Testing background audio playback ===')
        log(f'URL: {url}')

        if not self.adb.is_app_running(self.package):
            log(f'Launching Brave {self.label}...')
            self.adb.launch_app(self.package)
            time.sleep(5)

        log('Navigating to URL...')
        self.adb.launch_app(self.package, url=url)
        time.sleep(10)

        if not self._wait_for_youtube():
            log('[FAIL] YouTube page did not load in time.')
            log('DEBUG: UI dump (first 20 lines):')
            for line in self.ui.dump().splitlines()[:20]:
                print(line)
            return False

        # YouTube starts the video muted; tapping the player unmutes it.
        log('Tapping video player to unmute audio...')
        self.adb.tap(360, 420)
        time.sleep(2)

        log('Verifying audio output...')
        time.sleep(2)

        playing = False
        for attempt in range(1, AUDIO_FOREGROUND_ATTEMPTS + 1):
            if self._audio_active():
                playing = True
                log(f'Audio output confirmed (attempt {attempt}).')
                break
            time.sleep(AUDIO_FOREGROUND_INTERVAL)

        if not playing:
            log('[FAIL] Audio not detected after unmute.')
            self._dump_audio_debug()
            return False

        log('Moving Brave to background...')
        self.adb.keyevent(KEYCODE_HOME)
        time.sleep(AUDIO_BACKGROUND_SETTLE)

        passed = 0
        total = AUDIO_BACKGROUND_ATTEMPTS
        for attempt in range(1, total + 1):
            if self._audio_active():
                passed += 1
                log(f'  Background check {attempt}/{total}: audio active')
            else:
                log(f'  Background check {attempt}/{total}: audio NOT active')
            time.sleep(AUDIO_BACKGROUND_INTERVAL)

        if passed == total:
            log(' ✅ [PASS] Audio continues playing in the background.')
            return True

        log(f' ❌ [FAIL] Audio stopped when the app moved to the '
            f'background ({passed}/{total} checks passed).')
        self._dump_audio_debug()
        return False


# ---------------------------------------------------------------------------
# App data save / restore
# ---------------------------------------------------------------------------
def save_app_data(adb: Adb, package: str, destination: str) -> None:
    log('Enabling root access on emulator...')
    adb.run('root')
    time.sleep(2)

    log(f'Saving app data to {destination}...')
    adb.check('tar',
              'czf',
              DEVICE_BACKUP_PATH,
              '-C',
              '/data/data',
              package,
              error='Failed to create tar archive on device.')

    result = adb.run('pull', DEVICE_BACKUP_PATH, destination)
    if result.returncode != 0:
        raise SetupError('Failed to pull backup file from device.\n'
                         f'{result.stderr.strip()}')
    adb.shell('rm', DEVICE_BACKUP_PATH)

    if not os.path.isfile(destination) or os.path.getsize(destination) <= 100:
        raise SetupError('Backup file is empty or missing.')
    size = human_size(os.path.getsize(destination))
    log(f'App data saved to {destination} ({size})')


def restore_app_data(adb: Adb, package: str, source: str) -> None:
    log('Enabling root access on emulator...')
    adb.run('root')
    time.sleep(2)

    adb.shell('am', 'force-stop', package)
    time.sleep(1)

    log('Pushing backup to device...')
    result = adb.run('push', source, DEVICE_BACKUP_PATH)
    if result.returncode != 0:
        raise SetupError('Failed to push backup file to device.\n'
                         f'{result.stderr.strip()}')

    # Capture the app's UID so ownership can be fixed after the restore.
    app_uid = adb.out('stat', '-c', '%U', f'/data/data/{package}').strip()

    log('Restoring app data...')
    # Extract over the existing data dir (do NOT rm -rf first: the package
    # manager sets special group ownership and setgid bits on cache/ and
    # code_cache/ that must be preserved for the app to start correctly).
    adb.check('tar',
              'xzf',
              DEVICE_BACKUP_PATH,
              '-C',
              '/data/data',
              error='Failed to extract backup on device.')
    adb.shell('rm', DEVICE_BACKUP_PATH)

    if app_uid:
        log(f'Fixing file ownership to {app_uid}...')
        adb.shell('chown', '-R', f'{app_uid}:{app_uid}',
                  f'/data/data/{package}')

    # Restore SELinux contexts.
    adb.shell('restorecon', '-R', f'/data/data/{package}')

    log(f'App data restored from {source}')


# ---------------------------------------------------------------------------
# APK acquisition
# ---------------------------------------------------------------------------
def download_apk(release_filter: str, label: str, apk_pattern: str,
                 work_dir: str) -> str:
    log(f'Downloading latest Brave {label} APK from GitHub releases...')

    result = subprocess.run([
        'gh', 'release', 'list', '--repo', 'brave/brave-browser', '--limit',
        '20', '--json', 'tagName,name'
    ],
                            stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE,
                            text=True,
                            check=False)
    if result.returncode != 0:
        raise SetupError('Failed to list GitHub releases.\n'
                         f'{result.stderr.strip()}')

    try:
        releases = json.loads(result.stdout or '[]')
    except json.JSONDecodeError as error:
        raise SetupError(
            f'Could not parse `gh release list` output: {error}') from error

    # Try several releases in case the most recent one is still building.
    tags = [
        release['tagName'] for release in releases
        if re.search(release_filter, release.get('name', ''))
    ][:5]

    if not tags:
        raise SetupError(
            f'Could not find a {label} release. Provide an APK with --apk.')

    apk_path = os.path.join(work_dir, f'Brave{label}.apk')
    for tag in tags:
        log(f'Trying {tag} for {apk_pattern}...')
        download = subprocess.run([
            'gh', 'release', 'download', tag, '--repo', 'brave/brave-browser',
            '--pattern', apk_pattern, '--output', apk_path
        ],
                                  stdout=subprocess.DEVNULL,
                                  stderr=subprocess.DEVNULL,
                                  check=False)
        if download.returncode == 0:
            log(f'APK downloaded from {tag}')
            return apk_path

    raise SetupError(f'No {label} release has {apk_pattern}. '
                     'Provide an APK with --apk.')


# ---------------------------------------------------------------------------
# AVD setup
# ---------------------------------------------------------------------------
AVD_CONFIG_INI = """\
AvdId={avd_name}
PlayStore.enabled=false
abi.type=arm64-v8a
avd.ini.displayname=Brave {label} ATD (API 33)
avd.ini.encoding=UTF-8
disk.dataPartition.size=6G
fastboot.forceChosenSnapshotBoot=no
fastboot.forceColdBoot=no
fastboot.forceFastBoot=yes
hw.accelerometer=yes
hw.arc=false
hw.audioInput=yes
hw.audioOutput=yes
hw.battery=yes
hw.camera.back=none
hw.camera.front=none
hw.cpu.arch=arm64
hw.cpu.ncore=4
hw.dPad=no
hw.device.manufacturer=Google
hw.device.name=pixel_6
hw.gps=yes
hw.gpu.enabled=yes
hw.gpu.mode=auto
hw.initialOrientation=portrait
hw.keyboard=yes
hw.lcd.density=420
hw.lcd.height=2400
hw.lcd.width=1080
hw.mainKeys=no
hw.ramSize=4096
hw.sdCard=yes
hw.sensors.orientation=yes
hw.sensors.proximity=yes
hw.trackBall=no
image.sysdir.1=system-images/android-33/google_apis/arm64-v8a/
runtime.network.latency=none
runtime.network.speed=full
sdcard.size=512M
tag.display=Google APIs
tag.id=google_apis
"""


def setup_avd_x86_64() -> List[str]:
    """Install the CIPD google_atd x86_64 AVD (the canonical Chromium image).

    Returns extra emulator arguments.
    """
    log('x86_64 host detected - using CIPD google_atd x86_64 AVD.')
    for path, description in ((AVD_PY, 'avd.py'), (VPYTHON3, 'vpython3'),
                              (AVD_CONFIG, 'AVD config')):
        if not os.path.exists(path):
            raise SetupError(f'{description} not found at {path}')

    log(f'Running: avd.py install --avd-config {AVD_CONFIG}')
    result = subprocess.run([
        VPYTHON3, AVD_PY, 'install', '--avd-config', AVD_CONFIG, '--adb-path',
        ADB_PATH
    ],
                            check=False)
    if result.returncode != 0:
        raise SetupError('Failed to install AVD packages. '
                         'Check the error above.')
    log('AVD packages installed.')

    sysdir = os.path.join(CIPD_ROOT, 'system-images', 'android-33',
                          'google_atd', 'x86_64')
    if not os.path.isdir(sysdir):
        raise SetupError(f'System image not found at {sysdir}')
    if not os.path.isdir(os.path.join(CIPD_ROOT, 'avd')):
        raise SetupError(f'CIPD AVD not found at {CIPD_ROOT}/avd')

    # Symlink platform-tools from the real SDK so the CIPD root validates.
    for name in ('platforms', 'platform-tools'):
        link = os.path.join(CIPD_ROOT, name)
        target = os.path.join(ANDROID_SDK, name)
        if not os.path.exists(link) and os.path.isdir(target):
            os.symlink(target, link)

    os.environ['ANDROID_AVD_HOME'] = os.path.join(CIPD_ROOT, 'avd')
    os.environ['ANDROID_SDK_ROOT'] = CIPD_ROOT
    return ['-sysdir', sysdir]


def setup_avd_arm64(label: str) -> List[str]:
    """Create a local AVD with the arm64 system image (Apple Silicon).

    The ATD x86_64 system image cannot run on ARM hardware, so fall back to
    google_apis arm64-v8a for API 33.
    """
    log('ARM host detected - using local google_apis arm64-v8a system image.')
    system_image = 'system-images;android-33;google_apis;arm64-v8a'
    sysdir = os.path.join(ANDROID_SDK, 'system-images', 'android-33',
                          'google_apis', 'arm64-v8a')

    if not os.path.isdir(sysdir):
        log(f'System image not found. Installing {system_image}...')
        sdkmanager = os.path.join(ANDROID_SDK, 'cmdline-tools', 'latest',
                                  'bin', 'sdkmanager')
        result = subprocess.run([sdkmanager, '--install', system_image],
                                input='y\n' * 30,
                                stdout=subprocess.DEVNULL,
                                stderr=subprocess.DEVNULL,
                                text=True,
                                check=False)
        if result.returncode != 0:
            raise SetupError('Failed to install system image. Run: '
                             f"sdkmanager --install '{system_image}'")
    log(f'System image: {sysdir}')

    # Create the AVD by hand: avdmanager requires JDK 17+, which may be absent.
    avd_dir = os.path.expanduser('~/.android/avd')
    avd_ini = os.path.join(avd_dir, f'{AVD_NAME}.ini')
    avd_folder = os.path.join(avd_dir, f'{AVD_NAME}.avd')

    os.makedirs(avd_folder, exist_ok=True)

    if not os.path.isfile(avd_ini):
        log(f"Creating AVD '{AVD_NAME}'...")
        with open(avd_ini, 'w', encoding='utf-8') as ini:
            ini.write(f'avd.ini.encoding=UTF-8\n'
                      f'path={avd_folder}\n'
                      f'path.rel=avd/{AVD_NAME}.avd\n'
                      f'target=android-33\n')
    else:
        log(f"AVD '{AVD_NAME}' already exists, updating config...")

    # Always write config.ini so changes (audio, gpu, etc.) take effect.
    with open(os.path.join(avd_folder, 'config.ini'), 'w',
              encoding='utf-8') as config:
        config.write(AVD_CONFIG_INI.format(avd_name=AVD_NAME, label=label))

    return []


# ---------------------------------------------------------------------------
# Emulator lifecycle
# ---------------------------------------------------------------------------
def start_emulator(adb: Adb, extra_args: Sequence[str], headless: bool,
                   playback_test: bool) -> subprocess.Popen:
    log('=== Step 3: Starting emulator ===')

    # Kill any emulator already running on the default port.
    adb.run('emu', 'kill')
    time.sleep(2)

    args = [
        EMULATOR_PATH, '-avd', AVD_NAME, *extra_args, '-no-boot-anim',
        '-no-snapshot-save', '-wipe-data', '-memory', '4096'
    ]

    # Software rendering for playback tests: hardware-accelerated Vulkan on
    # Apple Silicon causes repeated VkDevice destroy/create cycles that crash
    # the renderer and reset the video player.
    if playback_test:
        args += ['-gpu', 'swiftshader_indirect']
    else:
        args += ['-gpu', 'auto', '-no-audio']

    if headless:
        args.append('-no-window')

    os.environ['ANDROID_EMULATOR_WAIT_TIME_BEFORE_KILL'] = '1'

    log(f"Launching: {' '.join(args)}")
    # The emulator deliberately outlives this call; the caller shuts it down
    # (or leaves it up for --keep-running), so `with` is not applicable.
    # pylint: disable=consider-using-with
    process = subprocess.Popen(args)
    log(f'Emulator started with PID {process.pid}')
    return process


def shutdown_emulator(adb: Adb, process: subprocess.Popen) -> None:
    log(f'Shutting down emulator (PID {process.pid})...')
    adb.run('emu', 'kill')
    try:
        process.wait(timeout=60)
    except subprocess.TimeoutExpired:
        process.kill()


# ---------------------------------------------------------------------------
# Argument parsing
# ---------------------------------------------------------------------------
EPILOG = """\
examples:
  %(prog)s --perform-onboarding --save-data script/brave_app_data.tar.gz
      Install Nightly (default), complete onboarding, save data, shut down.

  %(prog)s --channel release --perform-onboarding --keep-running
      Install the latest stable release, complete onboarding, keep running.

  %(prog)s --channel beta --apk ./BraveBeta.apk --keep-running
      Use a local Beta APK and leave the emulator running afterward.

  %(prog)s --load-data script/brave_app_data.tar.gz --keep-running
      Restore app data from a previous backup and keep the emulator running.

  %(prog)s --headless --perform-onboarding
      Run onboarding without an emulator window (CI-friendly).

  %(prog)s --load-data brave_data.tar.gz \\
      --check-playback-background "https://www.youtube.com/watch?v=dQw4w9WgXcQ" \\
      --keep-running
      Restore data, test background audio playback on a YouTube video.

  %(prog)s --save-data-now script/brave_app_data.tar.gz
      Save app data from an already running emulator without starting a new one.
"""


def parse_args(argv: Optional[Sequence[str]] = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description='Fires up an Android emulator using the '
        'android_33_google_atd_x64 AVD config and installs the latest Brave '
        'APK for the selected channel. Actions like onboarding and data '
        'save/load are opt-in via flags.',
        epilog=EPILOG,
        formatter_class=argparse.RawDescriptionHelpFormatter)

    parser.add_argument('--channel',
                        choices=sorted(CHANNELS),
                        default='nightly',
                        help='Brave channel to install. Default: nightly.')
    parser.add_argument('--apk',
                        dest='apk_path',
                        help='Use a local APK file instead of downloading the '
                        'latest release from GitHub.')
    parser.add_argument('--perform-onboarding',
                        action='store_true',
                        help='Complete the initial onboarding flow after '
                        'install.')
    parser.add_argument('--save-data',
                        help='Save app data to the given .tar.gz file.')
    parser.add_argument('--load-data',
                        help='Load previously saved app data from a .tar.gz '
                        'file.')
    parser.add_argument('--save-data-now',
                        help='Save app data from an already running emulator '
                        'to the given .tar.gz file, then exit. Does not start '
                        'a new emulator or install an APK.')
    parser.add_argument('--check-playback-background',
                        dest='playback_url',
                        metavar='URL',
                        help='Navigate to the given URL, wait for video/audio '
                        'to start playing, move the app to background, and '
                        'verify audio continues. Prints PASS or FAIL.')
    parser.add_argument('--keep-running',
                        action='store_true',
                        help='Keep the emulator running after setup is '
                        'complete. By default it is shut down when done.')
    parser.add_argument('--headless',
                        action='store_true',
                        help='Run the emulator without a graphical window '
                        '(useful for CI).')

    args = parser.parse_args(argv)

    if args.load_data and args.save_data:
        parser.error('--load-data and --save-data cannot be used together.')
    if args.load_data and args.perform_onboarding:
        parser.error('--load-data and --perform-onboarding cannot be used '
                     'together. Loaded data already contains a completed '
                     'onboarding state.')
    if args.load_data and not os.path.isfile(args.load_data):
        parser.error(f'Backup file not found: {args.load_data}')
    if args.apk_path and not os.path.isfile(args.apk_path):
        parser.error(f'APK file not found: {args.apk_path}')

    return args


# ---------------------------------------------------------------------------
# Entry points
# ---------------------------------------------------------------------------
def save_data_now(adb: Adb, package: str, destination: str) -> int:
    """Handle --save-data-now against an already running emulator."""
    if not adb.is_connected():
        raise SetupError(f'No emulator found on {SERIAL}. Is one running?')

    # Force-stop the app to flush data to disk.
    log(f'Stopping {package} to flush data...')
    adb.shell('am', 'force-stop', package)
    time.sleep(3)

    save_app_data(adb, package, destination)
    return 0


def print_summary(args: argparse.Namespace, channel: dict,
                  process: Optional[subprocess.Popen]) -> None:
    log()
    log('============================================================')
    log('  Setup complete!')
    log('============================================================')
    log()
    log(f"  Channel:            {channel['label']}")
    log(f'  AVD name:           {AVD_NAME}')
    log(f"  Package:            {channel['package']}")
    if args.load_data:
        log(f'  Restored data from: {args.load_data}')
    if args.save_data:
        log(f'  App data saved to:  {args.save_data}')
    if args.playback_url:
        log(f'  Playback test URL:  {args.playback_url}')
    log()
    if args.keep_running and process:
        log(f'  Emulator is still running (PID {process.pid}).')
        log(f'  Connect with: adb -s {SERIAL} shell')
        log(f'  Stop with:    adb -s {SERIAL} emu kill')
    else:
        log('  Emulator will be shut down.')
    log()


def run(args: argparse.Namespace) -> int:
    channel = CHANNELS[args.channel]
    package = channel['package']
    label = channel['label']

    adb = Adb(ADB_PATH)

    if args.save_data_now:
        return save_data_now(adb, package, args.save_data_now)

    # --- Step 1: Verify prerequisites ---
    log('=== Step 1: Verifying prerequisites ===')
    if not os.access(ADB_PATH, os.X_OK):
        raise SetupError(f'ADB not found at {ADB_PATH}')
    if not os.access(EMULATOR_PATH, os.X_OK):
        raise SetupError(f'Emulator not found at {EMULATOR_PATH}. '
                         'Install via Android SDK Manager.')

    host_arch = os.uname().machine
    log(f'  Channel:    {label} ({package})')
    log(f'  Host arch:  {host_arch}')
    log(f'  Emulator:   {EMULATOR_PATH}')
    log(f'  ADB:        {ADB_PATH}')

    # --- Step 2: Set up AVD ---
    log('=== Step 2: Setting up AVD ===')
    if host_arch == 'x86_64':
        extra_emulator_args = setup_avd_x86_64()
    else:
        extra_emulator_args = setup_avd_arm64(label)

    ui = Ui(adb)
    process: Optional[subprocess.Popen] = None
    work_dir: Optional[tempfile.TemporaryDirectory] = None

    try:
        # --- Step 3: Start emulator ---
        process = start_emulator(adb, extra_emulator_args, args.headless,
                                 bool(args.playback_url))
        adb.wait_for_device()
        adb.wait_for_boot()

        # Dismiss the initial Android setup / unlock.
        adb.keyevent(KEYCODE_MENU)
        time.sleep(2)
        adb.keyevent(KEYCODE_HOME)
        time.sleep(2)

        # --- Step 4: Download / locate Brave APK ---
        log(f'=== Step 4: Obtaining Brave {label} APK ===')
        # Cleaned up in the enclosing `finally`, which also has to survive the
        # early return from the playback check below.
        # pylint: disable=consider-using-with
        work_dir = tempfile.TemporaryDirectory()

        if args.apk_path:
            apk_path = args.apk_path
            log(f'Using provided APK: {apk_path}')
        else:
            # Pick the APK variant matching the emulator architecture.
            apk_pattern = ('BraveMonox64.apk'
                           if host_arch == 'x86_64' else 'BraveMonoarm64.apk')
            apk_path = download_apk(channel['release_filter'], label,
                                    apk_pattern, work_dir.name)

        # --- Step 5: Install APK ---
        log(f'=== Step 5: Installing Brave {label} ===')
        adb.run('uninstall', package)

        log('Installing APK (this may take a moment)...')
        install = adb.run('install', '-r', '-g', apk_path)
        if install.returncode != 0:
            raise SetupError('APK installation failed.\n'
                             f'{install.stderr.strip()}')
        log(f'Brave {label} installed.')

        # --- Step 6a: Load saved data ---
        if args.load_data:
            log('=== Step 6: Restoring app data ===')
            restore_app_data(adb, package, args.load_data)

            # Launch Brave to verify.
            time.sleep(2)
            adb.launch_app(package)
            time.sleep(5)

            if adb.is_app_running(package):
                log(f'Brave {label} is running with restored data.')
            else:
                log(f'WARNING: Brave {label} may not be running.')

        # --- Step 6b: Perform onboarding ---
        if args.perform_onboarding:
            log('=== Step 6: Launching Brave and completing onboarding ===')

            # Grant runtime permissions upfront to avoid permission dialogs.
            for permission in ('android.permission.POST_NOTIFICATIONS',
                               'android.permission.READ_EXTERNAL_STORAGE',
                               'android.permission.WRITE_EXTERNAL_STORAGE'):
                adb.shell('pm', 'grant', package, permission)

            adb.launch_app(package)
            log('Brave launched - waiting for onboarding to load...')
            time.sleep(8)

            onboarding = Onboarding(adb, ui)
            onboarding.dismiss_notification_permission()
            onboarding.run()

            # Dismiss any post-onboarding dialogs or notifications.
            time.sleep(3)
            onboarding.dismiss_notification_permission()

            # Press Home and return to ensure everything settles.
            adb.keyevent(KEYCODE_HOME)
            time.sleep(2)
            adb.launch_app(package)
            time.sleep(5)

            if adb.is_app_running(package):
                log(f'Brave {label} is running.')
            else:
                log(f'WARNING: Brave {label} may not be running.')

        # --- Step 6c: Check background playback ---
        if args.playback_url:
            check = PlaybackCheck(adb, ui, package, label)
            return 0 if check.run(args.playback_url) else 1

        # --- Step 7: Save app data ---
        if args.save_data:
            log('=== Step 7: Saving app data ===')

            # Force-stop the app to flush all data to disk.
            adb.shell('am', 'force-stop', package)
            time.sleep(3)

            save_app_data(adb, package, args.save_data)

        print_summary(args, channel, process)
    finally:
        if process and not args.keep_running:
            shutdown_emulator(adb, process)
        if work_dir:
            work_dir.cleanup()

    return 0


def main(argv: Optional[Sequence[str]] = None) -> int:
    # depot_tools must be on PATH so avd.py can find cipd, adb, etc.
    os.environ['PATH'] = DEPOT_TOOLS + os.pathsep + os.environ.get('PATH', '')
    # brave_chromium_utils must be importable by Chromium's patched catapult
    # modules.
    os.environ['PYTHONPATH'] = (SCRIPT_DIR + os.pathsep +
                                os.environ.get('PYTHONPATH', ''))

    args = parse_args(argv)
    try:
        return run(args)
    except SetupError as error:
        log(f'ERROR: {error}')
        return 1
    except KeyboardInterrupt:
        log('Interrupted.')
        return 130


if __name__ == '__main__':
    sys.exit(main())
