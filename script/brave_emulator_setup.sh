#!/usr/bin/env bash
# =============================================================================
# brave_emulator_setup.sh
#
# Fires up an Android emulator using the android_33_google_atd_x64 AVD config
# (via Chromium's avd.py tooling), installs the latest Brave APK for the
# selected channel (nightly/beta/release), and optionally completes the
# onboarding flow and/or saves/loads app data.
#
# Usage:
#   ./brave_emulator_setup.sh [OPTIONS]
#
# Options:
#   --apk <path>              Use a local APK instead of downloading the latest nightly
#   --perform-onboarding      Complete the initial onboarding flow after install
#   --save-data <file>        Save app data to the given .tar.gz file after setup
#   --load-data <file>        Load previously saved app data from the given .tar.gz file
#   --keep-running            Don't shut down the emulator after finishing
#   --headless                Run emulator without a window (no-window mode)
# =============================================================================
set -euo pipefail

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC_DIR="${SCRIPT_DIR}/../.."
AVD_PY="${SRC_DIR}/tools/android/avd/avd.py"
AVD_CONFIG="${SRC_DIR}/tools/android/avd/proto/android_33_google_atd_x64.textpb"
DEPOT_TOOLS="${SRC_DIR}/third_party/depot_tools"
VPYTHON3="${DEPOT_TOOLS}/vpython3"

# depot_tools must be on PATH so avd.py can find cipd, adb, etc.
export PATH="${DEPOT_TOOLS}:${PATH}"
# brave_chromium_utils must be importable by Chromium's patched catapult modules
export PYTHONPATH="${SCRIPT_DIR}:${PYTHONPATH:-}"

ANDROID_SDK="${ANDROID_HOME:-${HOME}/Library/Android/sdk}"
ADB="${ANDROID_SDK}/platform-tools/adb"

AVD_NAME="android_33_google_atd_x64"
# CIPD-installed AVD home (contains the AVD config and system images)
CIPD_ROOT="${SRC_DIR}/.android_emulator/${AVD_NAME}"
# Use the locally installed macOS emulator binary (CIPD packages are Linux-only)
EMULATOR="${ANDROID_SDK}/emulator/emulator"
BRAVE_ACTIVITY="com.google.android.apps.chrome.Main"

APK_PATH=""
CHANNEL="nightly"
LOAD_DATA=""
SAVE_DATA=""
SAVE_DATA_NOW=""
PERFORM_ONBOARDING=false
CHECK_PLAYBACK_BG=""
KEEP_RUNNING=false
HEADLESS=false
EMULATOR_PID=""
WORK_DIR=""

# ---------------------------------------------------------------------------
# Help
# ---------------------------------------------------------------------------
usage() {
    cat <<EOF
Usage: $(basename "$0") [OPTIONS]

Fires up an Android emulator using the android_33_google_atd_x64 AVD config
and installs the latest Brave APK for the selected channel.

Actions like onboarding and data save/load are opt-in via flags.

Options:
  --channel <name>          Brave channel to install: nightly, beta, or release.
                            Default: nightly.
  --apk <path>              Use a local APK file instead of downloading the
                            latest release from GitHub.
  --perform-onboarding      Complete the initial onboarding flow after install.
  --save-data <file>        Save app data to the given .tar.gz file.
                            Cannot be combined with --load-data.
  --load-data <file>        Load previously saved app data from a .tar.gz file.
                            Cannot be combined with --save-data or
                            --perform-onboarding.
  --save-data-now <file>    Save app data from an already running emulator to
                            the given .tar.gz file, then exit. Does not start
                            a new emulator or install an APK.
  --keep-running            Keep the emulator running after setup is complete.
                            By default the emulator is shut down when done.
  --check-playback-background <url>
                            Navigate to the given URL, wait for video/audio to
                            start playing, move the app to background, and
                            verify audio continues. Prints PASS or FAIL.
  --headless                Run the emulator without a graphical window
                            (useful for CI).
  --help                    Show this help message and exit.

Examples:
  $(basename "$0") --perform-onboarding --save-data script/brave_app_data.tar.gz
      Install Nightly (default), complete onboarding, save data, shut down.

  $(basename "$0") --channel release --perform-onboarding --keep-running
      Install the latest stable release, complete onboarding, keep running.

  $(basename "$0") --channel beta --apk ./BraveBeta.apk --keep-running
      Use a local Beta APK and leave the emulator running afterward.

  $(basename "$0") --load-data script/brave_app_data.tar.gz --keep-running
      Restore app data from a previous backup and keep the emulator running.

  $(basename "$0") --headless --perform-onboarding
      Run onboarding without an emulator window (CI-friendly).

  $(basename "$0") --load-data brave_data.tar.gz --check-playback-background "https://www.youtube.com/watch?v=dQw4w9WgXcQ" --keep-running
      Restore data, test background audio playback on a YouTube video.

  $(basename "$0") --save-data-now script/brave_app_data.tar.gz
      Save app data from an already running emulator without starting a new one.
EOF
    exit 0
}

# ---------------------------------------------------------------------------
# Argument parsing
# ---------------------------------------------------------------------------
while [[ $# -gt 0 ]]; do
    case "$1" in
        --help|-h)
            usage
            ;;
        --channel)
            CHANNEL="$2"
            shift 2
            ;;
        --apk)
            APK_PATH="$2"
            shift 2
            ;;
        --load-data)
            LOAD_DATA="$2"
            shift 2
            ;;
        --save-data)
            SAVE_DATA="$2"
            shift 2
            ;;
        --save-data-now)
            SAVE_DATA_NOW="$2"
            shift 2
            ;;
        --perform-onboarding)
            PERFORM_ONBOARDING=true
            shift
            ;;
        --check-playback-background)
            CHECK_PLAYBACK_BG="$2"
            shift 2
            ;;
        --keep-running)
            KEEP_RUNNING=true
            shift
            ;;
        --headless)
            HEADLESS=true
            shift
            ;;
        *)
            echo "Unknown option: $1"
            echo "Run '$(basename "$0") --help' for usage information."
            exit 1
            ;;
    esac
done

# ---------------------------------------------------------------------------
# Validate parameter combinations
# ---------------------------------------------------------------------------
# ---------------------------------------------------------------------------
# Derive channel-specific settings
# ---------------------------------------------------------------------------
case "$CHANNEL" in
    nightly)
        PACKAGE_NAME="com.brave.browser_nightly"
        CHANNEL_LABEL="Nightly"
        RELEASE_FILTER="Nightly"
        ;;
    beta)
        PACKAGE_NAME="com.brave.browser_beta"
        CHANNEL_LABEL="Beta"
        RELEASE_FILTER="Beta"
        ;;
    release)
        PACKAGE_NAME="com.brave.browser"
        CHANNEL_LABEL="Release"
        RELEASE_FILTER="Release"
        ;;
    *)
        echo "ERROR: Unknown channel '$CHANNEL'. Must be one of: nightly, beta, release."
        exit 1
        ;;
esac

if [[ -n "$LOAD_DATA" && -n "$SAVE_DATA" ]]; then
    echo "ERROR: --load-data and --save-data cannot be used together."
    exit 1
fi

if [[ -n "$LOAD_DATA" && "$PERFORM_ONBOARDING" == true ]]; then
    echo "ERROR: --load-data and --perform-onboarding cannot be used together."
    echo "       Loaded data already contains a completed onboarding state."
    exit 1
fi

if [[ -n "$LOAD_DATA" ]]; then
    [[ -f "$LOAD_DATA" ]] || { echo "ERROR: Backup file not found: $LOAD_DATA"; exit 1; }
fi

# ---------------------------------------------------------------------------
# Save data from a running emulator and exit (--save-data-now)
# ---------------------------------------------------------------------------
if [[ -n "$SAVE_DATA_NOW" ]]; then
    log() { echo "[$(date '+%H:%M:%S')] $*"; }
    die() { log "ERROR: $*"; exit 1; }

    # Verify an emulator is connected
    "$ADB" -s emulator-5554 get-state > /dev/null 2>&1 || \
        die "No emulator found on emulator-5554. Is one running?"

    # Force-stop the app to flush data to disk
    log "Stopping $PACKAGE_NAME to flush data..."
    "$ADB" -s emulator-5554 shell am force-stop "$PACKAGE_NAME"
    sleep 3

    log "Enabling root access on emulator..."
    "$ADB" -s emulator-5554 root
    sleep 2

    log "Saving app data to ${SAVE_DATA_NOW}..."
    "$ADB" -s emulator-5554 shell "tar czf /sdcard/brave_app_data.tar.gz -C /data/data ${PACKAGE_NAME}" || \
        die "Failed to create tar archive on device."
    "$ADB" -s emulator-5554 pull /sdcard/brave_app_data.tar.gz "$SAVE_DATA_NOW" || \
        die "Failed to pull backup file from device."
    "$ADB" -s emulator-5554 shell rm /sdcard/brave_app_data.tar.gz 2>/dev/null || true

    if [[ -f "$SAVE_DATA_NOW" && $(stat -f%z "$SAVE_DATA_NOW" 2>/dev/null || stat -c%s "$SAVE_DATA_NOW" 2>/dev/null || echo 0) -gt 100 ]]; then
        log "App data saved to $SAVE_DATA_NOW ($(du -h "$SAVE_DATA_NOW" | cut -f1))"
    else
        die "Backup file is empty or missing."
    fi
    exit 0
fi

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------
log() { echo "[$(date '+%H:%M:%S')] $*"; }
die() { log "ERROR: $*"; exit 1; }

cleanup() {
    if [[ "$KEEP_RUNNING" == false && -n "$EMULATOR_PID" ]]; then
        log "Shutting down emulator (PID $EMULATOR_PID)..."
        "$ADB" -s emulator-5554 emu kill 2>/dev/null || true
        wait "$EMULATOR_PID" 2>/dev/null || true
    fi
    if [[ -n "$WORK_DIR" && -d "$WORK_DIR" ]]; then
        rm -rf "$WORK_DIR"
    fi
}
trap cleanup EXIT

wait_for_boot() {
    local timeout=180
    local elapsed=0
    log "Waiting for emulator to boot (timeout: ${timeout}s)..."
    while [[ $elapsed -lt $timeout ]]; do
        local boot_completed
        boot_completed=$("$ADB" -s emulator-5554 shell getprop sys.boot_completed 2>/dev/null | tr -d '\r' || echo "")
        if [[ "$boot_completed" == "1" ]]; then
            log "Emulator booted successfully."
            return 0
        fi
        sleep 3
        elapsed=$((elapsed + 3))
    done
    die "Emulator failed to boot within ${timeout}s."
}

wait_for_device() {
    log "Waiting for device to come online..."
    "$ADB" wait-for-device
    sleep 2
}

# Parse UI XML to find center coordinates of an element.
# Usage: _find_center <xml_string> <attr_name> <attr_value>
# Outputs: "cx cy" or empty string if not found.
_find_center() {
    python3 -c "
import re, sys
xml = sys.argv[1]
attr = sys.argv[2]
val = sys.argv[3]
# Match node with the given attribute containing val, and extract bounds
pattern = attr + r'=\"[^\"]*' + re.escape(val) + r'\"[^>]*bounds=\"\[(\d+),(\d+)\]\[(\d+),(\d+)\]\"'
m = re.search(pattern, xml)
if m:
    x1, y1, x2, y2 = int(m.group(1)), int(m.group(2)), int(m.group(3)), int(m.group(4))
    print(f'{(x1+x2)//2} {(y1+y2)//2}')
" "$1" "$2" "$3" 2>/dev/null || echo ""
}

dump_ui() {
    "$ADB" -s emulator-5554 shell uiautomator dump /sdcard/ui_dump.xml 2>/dev/null || true
    "$ADB" -s emulator-5554 shell cat /sdcard/ui_dump.xml 2>/dev/null || echo ""
}

# Tap a UI element found by resource-id in the current view hierarchy.
# Usage: tap_by_id <resource_id_suffix> [timeout_seconds]
tap_by_id() {
    local id_suffix="$1"
    local timeout="${2:-30}"
    local elapsed=0

    while [[ $elapsed -lt $timeout ]]; do
        local xml
        xml=$(dump_ui)

        local coords
        coords=$(_find_center "$xml" "resource-id" "$id_suffix")
        if [[ -n "$coords" ]]; then
            local cx cy
            cx=${coords%% *}
            cy=${coords##* }
            log "Tapping '$id_suffix' at ($cx, $cy)"
            "$ADB" -s emulator-5554 shell input tap "$cx" "$cy"
            sleep 2
            return 0
        fi

        sleep 2
        elapsed=$((elapsed + 2))
    done

    log "WARNING: Element '$id_suffix' not found within ${timeout}s - skipping."
    return 1
}

# Tap a UI element found by its text content.
# Usage: tap_by_text <text> [timeout_seconds]
tap_by_text() {
    local text="$1"
    local timeout="${2:-30}"
    local elapsed=0

    while [[ $elapsed -lt $timeout ]]; do
        local xml
        xml=$(dump_ui)

        local coords
        coords=$(_find_center "$xml" "text" "$text")
        if [[ -n "$coords" ]]; then
            local cx cy
            cx=${coords%% *}
            cy=${coords##* }
            log "Tapping text '$text' at ($cx, $cy)"
            "$ADB" -s emulator-5554 shell input tap "$cx" "$cy"
            sleep 2
            return 0
        fi

        sleep 2
        elapsed=$((elapsed + 2))
    done

    log "WARNING: Text '$text' not found within ${timeout}s - skipping."
    return 1
}

# Check if a resource-id is currently visible in the UI.
is_visible() {
    local id_suffix="$1"
    local xml
    xml=$(dump_ui)
    echo "$xml" | grep -q "resource-id=\"[^\"]*${id_suffix}\""
}

# ---------------------------------------------------------------------------
# Onboarding flow functions
# ---------------------------------------------------------------------------

complete_onboarding() {
    local ui
    ui=$(dump_ui)

    # ---------- Handle any system "Set as default browser" dialog ----------
    if echo "$ui" | grep -q "android:id/button1"; then
        log "Detected system default browser dialog - dismissing..."
        tap_by_id "android:id/button1" 5 || true
        sleep 2
        ui=$(dump_ui)
    fi

    # ---------- Detect onboarding variant ----------
    if echo "$ui" | grep -q "onboarding_sure\|onboarding_later"; then
        log "Detected Variant Y onboarding"
        complete_variant_y
    elif echo "$ui" | grep -q "btn_positive\|btn_negative"; then
        log "Detected Variant X (default) onboarding"
        complete_variant_x
    else
        # Brave might still be loading or the splash animation is playing.
        log "Onboarding not yet visible. Waiting for splash animation..."
        sleep 10
        ui=$(dump_ui)

        if echo "$ui" | grep -q "onboarding_sure\|onboarding_later"; then
            log "Detected Variant Y onboarding (after splash)"
            complete_variant_y
        elif echo "$ui" | grep -q "btn_positive\|btn_negative"; then
            log "Detected Variant X (default) onboarding (after wait)"
            complete_variant_x
        else
            log "WARNING: Could not detect onboarding variant. Dumping UI for debug:"
            echo "$ui" | head -50
            log "Attempting generic fallback..."
            complete_generic_fallback
        fi
    fi
}

complete_variant_x() {
    # Step 0: "Set as Default Browser" - tap the positive button
    log "[Variant X] Step 0: Set as Default Browser"
    if is_visible "btn_positive"; then
        tap_by_id "btn_positive" 10 || true
        sleep 3
    fi

    # Handle OS-level default browser role request if it appears
    local ui
    ui=$(dump_ui)
    if echo "$ui" | grep -qi "default.*browser\|role.*request\|android.app.role"; then
        log "Dismissing OS default browser dialog..."
        "$ADB" -s emulator-5554 shell input keyevent 4  # BACK key
        sleep 2
    fi

    # Step 1: WDP (Web Discovery Project) - skip with "Maybe Later"
    log "[Variant X] Step 1: WDP page"
    if is_visible "btn_negative"; then
        tap_by_id "btn_negative" 10 || true
        sleep 3
    fi

    # Step 2: Analytics consent - accept with "Continue"
    log "[Variant X] Step 2: Analytics consent"
    if is_visible "btn_positive"; then
        tap_by_id "btn_positive" 10 || true
        sleep 3
    fi

    log "[Variant X] Onboarding completed."
}

complete_variant_y() {
    # Page 0: Help Brave Search - tap "Maybe Later" (skip WDP)
    log "[Variant Y] Page 0: Help Brave Search"
    tap_by_id "onboarding_later" 15 || tap_by_id "onboarding_sure" 5 || true
    sleep 3

    # Page 1: Block Interruptions - tap "Continue"
    log "[Variant Y] Page 1: Block Interruptions"
    tap_by_id "onboarding_continue" 15 || true
    sleep 3

    # Page 2: Make Brave Better - tap "Start Browsing"
    log "[Variant Y] Page 2: Make Brave Better"
    tap_by_id "onboarding_start_browsing" 15 || true
    sleep 3

    log "[Variant Y] Onboarding completed."
}

complete_generic_fallback() {
    log "Attempting fallback onboarding completion..."

    for i in 1 2 3 4 5; do
        local ui
        ui=$(dump_ui)

        # Try known button IDs
        for btn_id in "btn_positive" "btn_negative" "onboarding_sure" "onboarding_later" \
                      "onboarding_continue" "onboarding_start_browsing"; do
            if echo "$ui" | grep -q "$btn_id"; then
                tap_by_id "$btn_id" 5 || true
                sleep 3
                break
            fi
        done

        # Check if we've passed onboarding (look for browser chrome elements)
        ui=$(dump_ui)
        if echo "$ui" | grep -q "toolbar\|url_bar\|search_box\|tab_switcher\|home_button"; then
            log "Browser UI detected - onboarding appears complete."
            return 0
        fi

        # Try pressing back as a last resort
        "$ADB" -s emulator-5554 shell input keyevent 4
        sleep 2
    done
}

# Handle notification permission dialog if it appears
dismiss_notification_permission() {
    local ui
    ui=$(dump_ui)
    if echo "$ui" | grep -qi "allow\|notification"; then
        if echo "$ui" | grep -q "permission_allow_button\|com.android.permissioncontroller"; then
            log "Dismissing notification permission dialog..."
            tap_by_id "permission_allow_button" 5 || \
            tap_by_text "Allow" 5 || \
            tap_by_text "Don't allow" 5 || true
            sleep 2
        fi
    fi
}

# ---------------------------------------------------------------------------
# Step 1: Verify prerequisites
# ---------------------------------------------------------------------------
log "=== Step 1: Verifying prerequisites ==="
[[ -x "$ADB" ]]       || die "ADB not found at $ADB"
[[ -x "$EMULATOR" ]]  || die "Emulator not found at $EMULATOR. Install via Android SDK Manager."

HOST_ARCH=$(uname -m)
log "  Channel:    $CHANNEL_LABEL ($PACKAGE_NAME)"
log "  Host arch:  $HOST_ARCH"
log "  Emulator:   $EMULATOR"
log "  ADB:        $ADB"

# ---------------------------------------------------------------------------
# Step 2: Set up AVD
#
# On x86_64 hosts (Linux CI): use CIPD-installed google_atd x86_64 AVD via
#   avd.py install. This is the canonical Chromium testing image.
#
# On aarch64/arm64 hosts (Apple Silicon Mac): the ATD x86_64 system image
#   cannot run on ARM hardware. Fall back to creating a local AVD with
#   google_apis arm64-v8a system image for API 33.
# ---------------------------------------------------------------------------
log "=== Step 2: Setting up AVD ==="

if [[ "$HOST_ARCH" == "x86_64" ]]; then
    # --- x86_64 host: use CIPD ATD image ---
    log "x86_64 host detected - using CIPD google_atd x86_64 AVD."
    [[ -f "$AVD_PY" ]]      || die "avd.py not found at $AVD_PY"
    [[ -x "$VPYTHON3" ]]    || die "vpython3 not found at $VPYTHON3"
    [[ -f "$AVD_CONFIG" ]]  || die "AVD config not found at $AVD_CONFIG"

    log "Running: avd.py install --avd-config $AVD_CONFIG"
    "$VPYTHON3" "$AVD_PY" install --avd-config "$AVD_CONFIG" --adb-path "$ADB" || \
        die "Failed to install AVD packages. Check the error above."
    log "AVD packages installed."

    SYSDIR="${CIPD_ROOT}/system-images/android-33/google_atd/x86_64"
    [[ -d "$SYSDIR" ]] || die "System image not found at $SYSDIR"
    [[ -d "$CIPD_ROOT/avd" ]] || die "CIPD AVD not found at $CIPD_ROOT/avd"

    # Symlink platform-tools from real SDK so the CIPD root passes validation
    for dir in platforms platform-tools; do
        if [[ ! -e "$CIPD_ROOT/$dir" && -d "$ANDROID_SDK/$dir" ]]; then
            ln -s "$ANDROID_SDK/$dir" "$CIPD_ROOT/$dir"
        fi
    done

    export ANDROID_AVD_HOME="$CIPD_ROOT/avd"
    export ANDROID_SDK_ROOT="$CIPD_ROOT"
    EXTRA_EMULATOR_ARGS=(-sysdir "$SYSDIR")
else
    # --- arm64 host (Apple Silicon): create local AVD with arm64 image ---
    log "ARM host detected - using local google_apis arm64-v8a system image."
    LOCAL_SYSTEM_IMAGE="system-images;android-33;google_apis;arm64-v8a"
    LOCAL_SYSDIR="${ANDROID_SDK}/system-images/android-33/google_apis/arm64-v8a"

    if [[ ! -d "$LOCAL_SYSDIR" ]]; then
        log "System image not found. Installing ${LOCAL_SYSTEM_IMAGE}..."
        yes | "${ANDROID_SDK}/cmdline-tools/latest/bin/sdkmanager" --install "$LOCAL_SYSTEM_IMAGE" 2>/dev/null || \
            die "Failed to install system image. Run: sdkmanager --install '$LOCAL_SYSTEM_IMAGE'"
    fi
    log "System image: $LOCAL_SYSDIR"

    # Create AVD manually (avdmanager requires JDK 17+ which may not be present)
    AVD_DIR="${HOME}/.android/avd"
    AVD_INI="${AVD_DIR}/${AVD_NAME}.ini"
    AVD_FOLDER="${AVD_DIR}/${AVD_NAME}.avd"

    mkdir -p "$AVD_FOLDER"

    if [[ ! -f "$AVD_INI" ]]; then
        log "Creating AVD '$AVD_NAME'..."
        cat > "$AVD_INI" <<AVDINI
avd.ini.encoding=UTF-8
path=${AVD_FOLDER}
path.rel=avd/${AVD_NAME}.avd
target=android-33
AVDINI
    else
        log "AVD '$AVD_NAME' already exists, updating config..."
    fi

    # Always write config.ini so changes (audio, gpu, etc.) take effect.
    cat > "$AVD_FOLDER/config.ini" <<AVDCFG
AvdId=${AVD_NAME}
PlayStore.enabled=false
abi.type=arm64-v8a
avd.ini.displayname=Brave $CHANNEL_LABEL ATD (API 33)
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
AVDCFG

    EXTRA_EMULATOR_ARGS=()
fi

# ---------------------------------------------------------------------------
# Step 3: Start emulator
# ---------------------------------------------------------------------------
log "=== Step 3: Starting emulator ==="

# Kill any running emulators on the default port
"$ADB" -s emulator-5554 emu kill 2>/dev/null || true
sleep 2

EMULATOR_ARGS=(
    -avd "$AVD_NAME"
    ${EXTRA_EMULATOR_ARGS[@]+"${EXTRA_EMULATOR_ARGS[@]}"}
    -no-boot-anim
    -no-snapshot-save
    -wipe-data
    -memory 4096
)

# Use software rendering for playback tests — hardware-accelerated Vulkan on
# Apple Silicon causes repeated VkDevice destroy/create cycles that crash the
# renderer and reset the video player.
if [[ -n "$CHECK_PLAYBACK_BG" ]]; then
    EMULATOR_ARGS+=(-gpu swiftshader_indirect)
else
    EMULATOR_ARGS+=(-gpu auto -no-audio)
fi

if [[ "$HEADLESS" == true ]]; then
    EMULATOR_ARGS+=(-no-window)
fi

export ANDROID_EMULATOR_WAIT_TIME_BEFORE_KILL=1

log "Launching: $EMULATOR ${EMULATOR_ARGS[*]}"
"$EMULATOR" "${EMULATOR_ARGS[@]}" &
EMULATOR_PID=$!
log "Emulator started with PID $EMULATOR_PID"

wait_for_device
wait_for_boot

# Dismiss the initial Android setup / unlock
"$ADB" -s emulator-5554 shell input keyevent 82  # KEYCODE_MENU (unlock)
sleep 2
"$ADB" -s emulator-5554 shell input keyevent 3   # KEYCODE_HOME
sleep 2

# ---------------------------------------------------------------------------
# Step 4: Download / locate Brave APK
# ---------------------------------------------------------------------------
log "=== Step 4: Obtaining Brave $CHANNEL_LABEL APK ==="
WORK_DIR=$(mktemp -d)

if [[ -n "$APK_PATH" ]]; then
    [[ -f "$APK_PATH" ]] || die "APK file not found: $APK_PATH"
    log "Using provided APK: $APK_PATH"
else
    log "Downloading latest Brave $CHANNEL_LABEL APK from GitHub releases..."

    # Pick APK variant matching emulator architecture
    if [[ "$HOST_ARCH" == "x86_64" ]]; then
        APK_PATTERN="BraveMonox64.apk"
    else
        APK_PATTERN="BraveMonoarm64.apk"
    fi

    # Find releases for the selected channel (try several in case the latest is still building)
    RELEASE_TAGS=$(gh release list --repo brave/brave-browser --limit 20 --json tagName,name \
        --jq "[.[] | select(.name | test(\"${RELEASE_FILTER}\")) | .tagName] | .[:5] | .[]")

    if [[ -z "$RELEASE_TAGS" ]]; then
        die "Could not find a $CHANNEL_LABEL release. Provide an APK with --apk."
    fi

    APK_PATH="${WORK_DIR}/Brave${CHANNEL_LABEL}.apk"
    APK_DOWNLOADED=false

    while IFS= read -r tag; do
        log "Trying $tag for $APK_PATTERN..."
        if gh release download "$tag" \
            --repo brave/brave-browser \
            --pattern "$APK_PATTERN" \
            --output "$APK_PATH" 2>/dev/null; then
            log "APK downloaded from $tag"
            APK_DOWNLOADED=true
            break
        fi
    done <<< "$RELEASE_TAGS"

    if [[ "$APK_DOWNLOADED" != true ]]; then
        die "No $CHANNEL_LABEL release has $APK_PATTERN. Provide an APK with --apk."
    fi
fi

# ---------------------------------------------------------------------------
# Step 5: Install APK
# ---------------------------------------------------------------------------
log "=== Step 5: Installing Brave $CHANNEL_LABEL ==="

# Uninstall any previous version
"$ADB" -s emulator-5554 uninstall "$PACKAGE_NAME" 2>/dev/null || true

log "Installing APK (this may take a moment)..."
"$ADB" -s emulator-5554 install -r -g "$APK_PATH" || die "APK installation failed."
log "Brave $CHANNEL_LABEL installed."

# ---------------------------------------------------------------------------
# Step 6a: Load saved data (if --load-data)
# ---------------------------------------------------------------------------
if [[ -n "$LOAD_DATA" ]]; then
    log "=== Step 6: Restoring app data ==="

    # Use adb root + tar to restore app data directly
    log "Enabling root access on emulator..."
    "$ADB" -s emulator-5554 root
    sleep 2

    # Force-stop the app before restoring data
    "$ADB" -s emulator-5554 shell am force-stop "$PACKAGE_NAME"
    sleep 1

    log "Pushing backup to device..."
    "$ADB" -s emulator-5554 push "$LOAD_DATA" /sdcard/brave_app_data.tar.gz || \
        die "Failed to push backup file to device."

    # Get the app's UID for fixing ownership after restore
    APP_UID=$("$ADB" -s emulator-5554 shell stat -c '%U' "/data/data/${PACKAGE_NAME}" 2>/dev/null || echo "")

    log "Restoring app data..."
    # Extract over the existing data dir (do NOT rm -rf first — the package
    # manager sets special group ownership and setgid bits on cache/ and
    # code_cache/ that must be preserved for the app to start correctly).
    "$ADB" -s emulator-5554 shell "tar xzf /sdcard/brave_app_data.tar.gz -C /data/data" || \
        die "Failed to extract backup on device."
    "$ADB" -s emulator-5554 shell rm /sdcard/brave_app_data.tar.gz 2>/dev/null || true

    # Fix ownership — the UID may differ between installs
    if [[ -n "$APP_UID" ]]; then
        log "Fixing file ownership to $APP_UID..."
        "$ADB" -s emulator-5554 shell "chown -R ${APP_UID}:${APP_UID} /data/data/${PACKAGE_NAME}"
    fi

    # Restore SELinux contexts
    "$ADB" -s emulator-5554 shell "restorecon -R /data/data/${PACKAGE_NAME}" 2>/dev/null || true

    log "App data restored from $LOAD_DATA"

    # Launch Brave to verify
    sleep 2
    "$ADB" -s emulator-5554 shell am start -n "${PACKAGE_NAME}/${BRAVE_ACTIVITY}" \
        -a android.intent.action.MAIN -c android.intent.category.LAUNCHER
    sleep 5

    if "$ADB" -s emulator-5554 shell pidof "$PACKAGE_NAME" > /dev/null 2>&1; then
        log "Brave $CHANNEL_LABEL is running with restored data."
    else
        log "WARNING: Brave $CHANNEL_LABEL may not be running."
    fi
fi

# ---------------------------------------------------------------------------
# Step 6b: Perform onboarding (if --perform-onboarding)
# ---------------------------------------------------------------------------
if [[ "$PERFORM_ONBOARDING" == true ]]; then
    log "=== Step 6: Launching Brave and completing onboarding ==="

    # Grant all runtime permissions upfront to avoid permission dialogs
    "$ADB" -s emulator-5554 shell pm grant "$PACKAGE_NAME" android.permission.POST_NOTIFICATIONS 2>/dev/null || true
    "$ADB" -s emulator-5554 shell pm grant "$PACKAGE_NAME" android.permission.READ_EXTERNAL_STORAGE 2>/dev/null || true
    "$ADB" -s emulator-5554 shell pm grant "$PACKAGE_NAME" android.permission.WRITE_EXTERNAL_STORAGE 2>/dev/null || true

    # Launch Brave
    "$ADB" -s emulator-5554 shell am start -n "${PACKAGE_NAME}/${BRAVE_ACTIVITY}" \
        -a android.intent.action.MAIN -c android.intent.category.LAUNCHER
    log "Brave launched - waiting for onboarding to load..."
    sleep 8

    # Run onboarding flow
    dismiss_notification_permission
    complete_onboarding

    # Dismiss any post-onboarding dialogs or notifications
    sleep 3
    dismiss_notification_permission

    # Press Home and return to ensure everything settles
    "$ADB" -s emulator-5554 shell input keyevent 3  # HOME
    sleep 2
    "$ADB" -s emulator-5554 shell am start -n "${PACKAGE_NAME}/${BRAVE_ACTIVITY}" \
        -a android.intent.action.MAIN -c android.intent.category.LAUNCHER
    sleep 5

    # Verify browser is running
    if "$ADB" -s emulator-5554 shell pidof "$PACKAGE_NAME" > /dev/null 2>&1; then
        log "Brave $CHANNEL_LABEL is running."
    else
        log "WARNING: Brave $CHANNEL_LABEL may not be running."
    fi
fi

# ---------------------------------------------------------------------------
# Step 6c: Check background playback (if --check-playback-background)
# ---------------------------------------------------------------------------
if [[ -n "$CHECK_PLAYBACK_BG" ]]; then
    log "=== Step 6c: Testing background audio playback ==="
    log "URL: $CHECK_PLAYBACK_BG"

    # Helper: check if audio is actively being output by inspecting the
    # primary output mixer in audio_flinger. When audio plays, the primary
    # output thread has Standby=no and Last write < 1000ms. When paused or
    # stopped, it goes to Standby=yes with a stale Last write timestamp.
    _is_audio_playing() {
        "$ADB" -s emulator-5554 shell dumpsys media.audio_flinger 2>/dev/null | \
            python3 -c "
import sys, re
text = sys.stdin.read()
# Find the primary output thread (AUDIO_DEVICE_OUT_SPEAKER)
blocks = re.split(r'(?=Output thread)', text)
for b in blocks:
    if 'AUDIO_DEVICE_OUT_SPEAKER' not in b:
        continue
    standby = re.search(r'Standby:\s*(yes|no)', b)
    last_write = re.search(r'Last write occurred \(msecs\):\s*(\d+)', b)
    if standby and standby.group(1) == 'no' and last_write and int(last_write.group(1)) < 1000:
        sys.exit(0)
sys.exit(1)
" 2>/dev/null
    }

    # Helper: dump audio debug info.
    _dump_audio_debug() {
        log "DEBUG: audio_flinger primary output:"
        "$ADB" -s emulator-5554 shell dumpsys media.audio_flinger 2>/dev/null | \
            grep -E "Standby:|Last write occurred|AUDIO_DEVICE_OUT_SPEAKER" || echo "(none)"
    }

    # Ensure Brave is running
    if ! "$ADB" -s emulator-5554 shell pidof "$PACKAGE_NAME" > /dev/null 2>&1; then
        log "Launching Brave $CHANNEL_LABEL..."
        "$ADB" -s emulator-5554 shell am start -n "${PACKAGE_NAME}/${BRAVE_ACTIVITY}" \
            -a android.intent.action.MAIN -c android.intent.category.LAUNCHER
        sleep 5
    fi

    # Navigate to the URL via intent (opens in Brave)
    log "Navigating to URL..."
    "$ADB" -s emulator-5554 shell am start -n "${PACKAGE_NAME}/${BRAVE_ACTIVITY}" \
        -a android.intent.action.VIEW -d "$CHECK_PLAYBACK_BG"
    sleep 10

    # YouTube plays the video muted. Tapping anywhere on the video player
    # unmutes it. We wait for the page to load, then tap the center of the
    # video area to unmute.
    #
    # First, wait for the video to appear by looking for YouTube player
    # elements in the UI (e.g. "Subscribe", video title, etc.).
    log "Waiting for YouTube video to load..."
    VIDEO_LOADED=false
    for attempt in $(seq 1 15); do
        xml=$(dump_ui)
        if echo "$xml" | grep -qi "Subscribe\|views\|youtube"; then
            VIDEO_LOADED=true
            log "YouTube page loaded (attempt $attempt)."
            break
        fi
        sleep 2
    done

    if [[ "$VIDEO_LOADED" == false ]]; then
        log "[FAIL] YouTube page did not load in time."
        log "DEBUG: UI dump (first 20 lines):"
        dump_ui | head -20
    else
        # Tap the center of the video player to unmute.
        log "Tapping video player to unmute audio..."
        "$ADB" -s emulator-5554 shell input tap 360 420
        sleep 2

        # Verify audio is now playing via dumpsys audio.
        log "Verifying audio output..."
        sleep 2

        PLAYING=false
        for attempt in $(seq 1 5); do
            if _is_audio_playing; then
                PLAYING=true
                log "Audio output confirmed (attempt $attempt)."
                break
            fi
            sleep 2
        done

        if [[ "$PLAYING" == false ]]; then
            log "[FAIL] Audio not detected after unmute."
            _dump_audio_debug
        else
            log "Moving Brave to background..."
            "$ADB" -s emulator-5554 shell input keyevent 3  # HOME
            sleep 5

            # Verify audio is still playing in the background.
            BG_PLAYING=false
            for attempt in $(seq 1 3); do
                if _is_audio_playing; then
                    BG_PLAYING=true
                    break
                fi
                sleep 3
            done

            if [[ "$BG_PLAYING" == true ]]; then
                log " ✅ [PASS] Audio continues playing in the background."
                exit 0
            else
                log " ❌ [FAIL] Audio stopped when the app moved to the background."
                _dump_audio_debug
                exit 1
            fi
        fi
    fi
fi

# ---------------------------------------------------------------------------
# Step 7: Save app data (if --save-data)
# ---------------------------------------------------------------------------
if [[ -n "$SAVE_DATA" ]]; then
    log "=== Step 7: Saving app data ==="

    # Force-stop the app to flush all data to disk
    "$ADB" -s emulator-5554 shell am force-stop "$PACKAGE_NAME"
    sleep 3

    BACKUP_FILE="$SAVE_DATA"

    # Use adb root + tar to save app data directly (adb backup is unreliable)
    log "Enabling root access on emulator..."
    "$ADB" -s emulator-5554 root
    sleep 2

    log "Saving app data to ${BACKUP_FILE}..."
    "$ADB" -s emulator-5554 shell "tar czf /sdcard/brave_app_data.tar.gz -C /data/data ${PACKAGE_NAME}" || \
        die "Failed to create tar archive on device."
    "$ADB" -s emulator-5554 pull /sdcard/brave_app_data.tar.gz "$BACKUP_FILE" || \
        die "Failed to pull backup file from device."
    "$ADB" -s emulator-5554 shell rm /sdcard/brave_app_data.tar.gz 2>/dev/null || true

    if [[ -f "$BACKUP_FILE" && $(stat -f%z "$BACKUP_FILE" 2>/dev/null || stat -c%s "$BACKUP_FILE" 2>/dev/null || echo 0) -gt 100 ]]; then
        log "App data saved to $BACKUP_FILE ($(du -h "$BACKUP_FILE" | cut -f1))"
    else
        die "Backup file is empty or missing."
    fi
fi

# ---------------------------------------------------------------------------
# Summary
# ---------------------------------------------------------------------------
log ""
log "============================================================"
log "  Setup complete!"
log "============================================================"
log ""
log "  Channel:            $CHANNEL_LABEL"
log "  AVD name:           $AVD_NAME"
log "  Package:            $PACKAGE_NAME"
if [[ -n "$LOAD_DATA" ]]; then
    log "  Restored data from: $LOAD_DATA"
fi
if [[ -n "$SAVE_DATA" ]]; then
    log "  App data saved to:  $SAVE_DATA"
fi
if [[ -n "$CHECK_PLAYBACK_BG" ]]; then
    log "  Playback test URL:  $CHECK_PLAYBACK_BG"
fi
log ""
if [[ "$KEEP_RUNNING" == true ]]; then
    log "  Emulator is still running (PID $EMULATOR_PID)."
    log "  Connect with: adb -s emulator-5554 shell"
    log "  Stop with:    adb -s emulator-5554 emu kill"
else
    log "  Emulator will be shut down."
fi
log ""
