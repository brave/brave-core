#!/usr/bin/env bash
# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

# Helper script to generate SHA256 checksums for all opengrep binaries
# and automatically update the install_opengrep.py script

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
INSTALL_SCRIPT="${SCRIPT_DIR}/install_opengrep.py"

# Check if sed supports -i without backup file (GNU vs BSD)
if sed --version >/dev/null 2>&1; then
    # GNU sed
    SED_INPLACE=(sed -i)
else
    # BSD sed (macOS)
    SED_INPLACE=(sed -i '')
fi

VERSION="${1:-v1.11.2}"

echo "==========================================="
echo "Opengrep Checksum Generator & Updater"
echo "==========================================="
echo
echo "Version: ${VERSION}"
echo "Target:  ${INSTALL_SCRIPT}"
echo

# Create temporary directory
TEMP_DIR=$(mktemp -d)
trap 'rm -rf "${TEMP_DIR}"' EXIT

cd "${TEMP_DIR}"

# All supported platforms
# Note: Windows binaries have .exe in the distribution name
# Note: Windows ARM64 is not available
PLATFORMS=(
    "opengrep_osx_arm64"
    "opengrep_osx_x86"
    "opengrep_manylinux_x86"
    "opengrep_manylinux_aarch64"
    "opengrep_musllinux_x86"
    "opengrep_musllinux_aarch64"
    "opengrep_windows_x86.exe"
)

echo "Step 1: Downloading binaries and signatures..."
DOWNLOADED_COUNT=0
for DIST in "${PLATFORMS[@]}"; do
    BASE_URL="https://github.com/opengrep/opengrep/releases/download"
    URL="${BASE_URL}/${VERSION}/${DIST}"
    echo "  - ${DIST}..."

    if curl -fsSL -o "${DIST}" "${URL}"; then
        DOWNLOADED_COUNT=$((DOWNLOADED_COUNT + 1))

        # Also download signature files if they exist
        SIG_URL="${URL}.sig"
        CERT_URL="${URL}.cert"

        if curl -fsSL -o "${DIST}.sig" "${SIG_URL}" 2>/dev/null; then
            echo "    ✓ Downloaded ${DIST}.sig"
        fi

        if curl -fsSL -o "${DIST}.cert" "${CERT_URL}" 2>/dev/null; then
            echo "    ✓ Downloaded ${DIST}.cert"
        fi
    else
        echo "    Warning: Failed to download ${DIST}"
        echo "             (may not exist for this version)"
        rm -f "${DIST}"
    fi
done

if [ ${DOWNLOADED_COUNT} -eq 0 ]; then
    echo
    echo "Error: No binaries could be downloaded. Check the version number."
    exit 1
fi

echo
echo "✓ Downloaded ${DOWNLOADED_COUNT}/${#PLATFORMS[@]} binaries"
echo

echo "Step 2: Generating checksums..."
declare -A CHECKSUMS

for DIST in opengrep_* *windows*.exe; do
    if [ -f "${DIST}" ]; then
        # Calculate SHA256 of the binary
        if command -v sha256sum > /dev/null 2>&1; then
            CHECKSUM=$(sha256sum "${DIST}" | cut -d' ' -f1)
        elif command -v shasum > /dev/null 2>&1; then
            CHECKSUM=$(shasum -a 256 "${DIST}" | cut -d' ' -f1)
        else
            echo "Error: Neither sha256sum nor shasum found"
            exit 1
        fi

        CHECKSUMS["${DIST}"]="${CHECKSUM}"
        echo "  ✓ ${DIST}: ${CHECKSUM}"

        # Also checksum the signature file if it exists
        if [ -f "${DIST}.sig" ]; then
            if command -v sha256sum > /dev/null 2>&1; then
                SIG_CHECKSUM=$(sha256sum "${DIST}.sig" | cut -d' ' -f1)
            elif command -v shasum > /dev/null 2>&1; then
                SIG_CHECKSUM=$(shasum -a 256 "${DIST}.sig" | cut -d' ' -f1)
            fi
            echo "    (signature: ${SIG_CHECKSUM})"
        fi
    fi
done

echo
echo "Step 3: Updating ${INSTALL_SCRIPT}..."

# Backup the original file
cp "${INSTALL_SCRIPT}" "${INSTALL_SCRIPT}.bak"
echo "  - Created backup: ${INSTALL_SCRIPT}.bak"

# Update version
"${SED_INPLACE[@]}" \
  "s/^OPENGREP_VERSION = .*/OPENGREP_VERSION = '${VERSION}'/" \
  "${INSTALL_SCRIPT}"
echo "  ✓ Updated OPENGREP_VERSION to '${VERSION}'"

# Update each checksum in BINARY_CHECKSUMS dictionary
# Format: 'platform': (\n        'checksum'),
for DIST in "${PLATFORMS[@]}"; do
    if [ -n "${CHECKSUMS[${DIST}]:-}" ]; then
        CHECKSUM="${CHECKSUMS[${DIST}]}"
        # Use perl for multi-line replacement to maintain 80-char limit
        # Match: 'dist': (\n        'old_checksum'),
        # Replace with: 'dist': (\n        'new_checksum'),
        perl -i -pe "
          if (/'${DIST}': \(/) {
            \$_ .= <>;
            s/('${DIST}': \(\\n\\s+')[^']*('\\),)/\${1}${CHECKSUM}\${2}/;
          }
        " "${INSTALL_SCRIPT}"
        echo "  ✓ Updated ${DIST}"
    else
        echo "  - Skipped ${DIST} (not downloaded)"
    fi
done

echo
echo "Step 4: Verifying changes..."

# Show the updated BINARY_CHECKSUMS section
echo
echo "Updated BINARY_CHECKSUMS section:"
echo "-----------------------------------"
sed -n '/^BINARY_CHECKSUMS = {/,/^}/p' "${INSTALL_SCRIPT}"
echo "-----------------------------------"
echo

# Verify Python syntax
if command -v python3 > /dev/null 2>&1; then
    if python3 -m py_compile "${INSTALL_SCRIPT}"; then
        echo "✓ Python syntax validation passed"
    else
        echo "Error: Python syntax validation failed"
        echo "Restoring backup..."
        mv "${INSTALL_SCRIPT}.bak" "${INSTALL_SCRIPT}"
        exit 1
    fi
else
    echo "Warning: python3 not found, skipping syntax validation"
fi

echo
echo "==========================================="
echo "✓ Successfully updated ${INSTALL_SCRIPT}"
echo "==========================================="
echo
echo "Changes:"
echo "  - Version: ${VERSION}"
echo "  - Updated ${#CHECKSUMS[@]} checksums"
echo
echo "Signature files available for verification:"
for DIST in "${PLATFORMS[@]}"; do
    if [ -f "${DIST}.sig" ]; then
        echo "  ✓ ${DIST}.sig"
    fi
done
echo
echo "Backup saved to: ${INSTALL_SCRIPT}.bak"
echo
echo "To test the installation, run:"
echo "  vpython3 ${INSTALL_SCRIPT}"
echo
echo "To remove backup:"
echo "  rm ${INSTALL_SCRIPT}.bak"
