#!/usr/bin/env python3
# Copyright (c) 2023 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Creates symsrv directory structure with executables and pdb files."""

import argparse
import asyncio
import concurrent.futures
import configparser
import os
import re
import shutil
import subprocess

from datetime import datetime

ROOT_DIR = os.path.join(os.path.dirname(__file__), *[os.pardir] * 3)


async def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '--installer-config',
        required=True,
        help='Installer config that contains files to process.',
    )
    parser.add_argument(
        '--additional-files',
        nargs='+',
        help='Additional exe/dll files to process.',
    )
    parser.add_argument(
        '--build-dir',
        required=True,
        help='Build directory.',
    )
    parser.add_argument(
        '--toolchain-dir',
        required=True,
        help='Toolchain directory.',
    )
    parser.add_argument(
        '--symbols-dir',
        required=True,
        help='Directory to output all files for symbol server.',
    )
    parser.add_argument(
        '--run-source-index',
        default=False,
        action='store_true',
        help="""
          Run source_index.py from the Chromium toolset. This tool will take a
          PDB on the command line, extract the source files that were used in
          building the PDB, query the source server for which repository and
          revision these files are at, and then finally write this information
          back into the PDB in a format that the debugging tools understand.
          This allows for automatic source debugging, as all of the information
          is contained in the PDB, and the debugger can go out and fetch the
          source files.
        """,
    )
    parser.add_argument(
        '--clear',
        default=False,
        action='store_true',
        help='Clear the symbol directories before writing new symbols.',
    )

    args = parser.parse_intermixed_args()

    if args.clear:
        shutil.rmtree(args.symbols_dir, ignore_errors=True)

    tasks = [
        run_with_semaphore(process_image(args, image_path))
        for image_path in get_images_with_pdbs(args)
    ]
    for result in asyncio.as_completed(tasks):
        print(await result)


def get_images_with_pdbs(args):
    images_with_pdbs = []

    def add_if_has_pdb(file, should_exist=False):
        file = os.path.join(args.build_dir, file)
        if not os.path.exists(file):
            if should_exist:
                raise RuntimeError(f'{file} not found')
            return
        pdb_file = file + '.pdb'
        if not os.path.exists(pdb_file):
            return
        assert file not in images_with_pdbs
        images_with_pdbs.append(file)

    installer_config = configparser.ConfigParser()
    installer_config.optionxform = str  # Preserve string case.
    installer_config.read(args.installer_config)
    for group in installer_config:
        if group == 'GOOGLE_CHROME':
            # Skip Google Chrome-only files.
            continue

        for file in installer_config[group]:
            add_if_has_pdb(file)

    if args.additional_files:
        for file in args.additional_files:
            add_if_has_pdb(file, True)

    # Sanity check to ensure we're not missing chrome.dll.pdb.
    assert any(
        images_with_pdb.endswith('chrome.dll')
        for images_with_pdb in images_with_pdbs), "chrome.dll.pdb not found"

    return images_with_pdbs


async def process_image(args, image_path):
    start_time = datetime.utcnow()
    output = f'Processing {image_path}'

    image_fingerprint, (image_pdb_fingerprint, pdb_path) = await asyncio.gather(
        get_img_fingerprint(image_path),
        get_pdb_info_from_img(image_path),
    )
    output += f'\n{image_fingerprint=!s}'

    pdb_fingerprint = await get_pdb_fingerprint(pdb_path)
    output += f'\n{pdb_fingerprint=!s}'

    if image_pdb_fingerprint != pdb_fingerprint:
        raise RuntimeError(
            f"Image PDB fingerprint doesn't match PDB fingerprint:\n"
            f"{image_pdb_fingerprint} : {image_path}\n"
            f"{pdb_fingerprint} : {pdb_path}")

    await copy_symbol(image_path, image_fingerprint, args.symbols_dir)
    copied_pdb_path = await copy_symbol(pdb_path, pdb_fingerprint,
                                        args.symbols_dir)

    if args.run_source_index:
        run_source_index_result = await run_source_index(args, copied_pdb_path)
        output += '\n' + run_source_index_result.strip()

    elapsed = datetime.utcnow() - start_time
    output += (f'\nCompleted. Elapsed time {elapsed.total_seconds()} seconds')
    return output


async def get_img_fingerprint(image_path):
    output = await check_output([
        'python3.bat',
        os.path.join(ROOT_DIR, 'tools', 'symsrc', 'img_fingerprint.py'),
        image_path,
    ])
    return output.strip()


async def get_pdb_info_from_img(image_path):
    output = await check_output([
        'python3.bat',
        os.path.join(ROOT_DIR, 'tools', 'symsrc',
                     'pdb_fingerprint_from_img.py'),
        image_path,
    ])
    fingerprint, filename = output.strip().split(' ', 1)
    return fingerprint, filename


async def get_pdb_fingerprint(pdb_path):
    llvm_pdbutil_path = os.path.join(ROOT_DIR, 'third_party', 'llvm-build',
                                     'Release+Asserts', 'bin',
                                     'llvm-pdbutil.exe')
    assert os.path.exists(llvm_pdbutil_path)

    stdout = await check_output(
        [llvm_pdbutil_path, 'dump', '--summary', pdb_path])

    guid_match = None
    for line in stdout.splitlines():
        if not guid_match:
            guid_match = re.match(r'  GUID: {(.*)}', line)
            if guid_match:
                break

    if not guid_match:
        raise RuntimeError(
            f'GUID is not found in llvm-pdbutil output for {pdb_path}:\n'
            f'{stdout}')

    # DBI stream age is always `1` for non-incremental builds.
    dbi_age = '1'

    # PDB fingerprint is a concat of GUID and DBI stream age.
    return guid_match.group(1).replace('-', '') + dbi_age


async def copy_symbol(symbol_path, symbol_fingerprint, dest_symbols_dir):
    symbol_name = os.path.basename(symbol_path)
    dest_symbol_dir = os.path.join(dest_symbols_dir, symbol_name,
                                   symbol_fingerprint)
    os.makedirs(dest_symbol_dir, exist_ok=True)
    dest_symbol_path = os.path.join(dest_symbol_dir, symbol_name)
    await run_on_thread_pool(shutil.copy, symbol_path, dest_symbol_path)

    return dest_symbol_path


async def run_source_index(args, pdb_path):
    # Inject source server information into PDBs to allow VS/WinDBG
    # automatically fetch sources from GitHub.
    #
    # Note: this will increase PDB age, but not DBI age. The PDB has an age
    # value in the header and also an age value in the DBI stream. When linking,
    # the linker increments the header’s age and copies the new value to the DBI
    # age. Pdbstr increments the header age but doesn’t touch the DBI age. The
    # DBI age is the value matched against the requested age from the binary
    # being debugged, which is why nothing breaks.
    # https://randomascii.wordpress.com/2011/11/11/source-indexing-is-underused-awesomeness/#comment-34328

    source_index_env = {
        **os.environ,
        # The file list output of `pdbstr.exe` is put into set(). Disable hash
        # randomization to keep the file order stable between runs.
        'PYTHONHASHSEED': '0',
    }
    result = await check_output([
        'vpython3.bat',
        os.path.join(ROOT_DIR, 'tools', 'symsrc', 'source_index.py'),
        '--build-dir',
        args.build_dir,
        '--toolchain-dir',
        args.toolchain_dir,
        pdb_path,
    ],
                                env=source_index_env)
    return result.strip()


async def run_with_semaphore(coro):
    if not hasattr(run_with_semaphore, 'semaphore'):
        run_with_semaphore.semaphore = asyncio.Semaphore(
            max(1,
                os.cpu_count() / 2))
    async with run_with_semaphore.semaphore:
        return await coro


async def run_on_thread_pool(*args, **kwargs):
    if not hasattr(run_on_thread_pool, 'instance'):
        run_on_thread_pool.instance = concurrent.futures.ThreadPoolExecutor()
    return await asyncio.wrap_future(
        run_on_thread_pool.instance.submit(*args, **kwargs))


async def check_output(args, encoding='oem', **kwargs):
    proc = await asyncio.create_subprocess_exec(
        *args,
        stdout=asyncio.subprocess.PIPE,
        stderr=asyncio.subprocess.PIPE,
        **kwargs,
    )
    stdout, stderr = await proc.communicate()
    stdout = stdout.decode(encoding)
    stderr = stderr.decode(encoding)
    if proc.returncode:
        raise subprocess.CalledProcessError(proc.returncode,
                                            args,
                                            output=stdout,
                                            stderr=stderr)
    return stdout


if __name__ == '__main__':
    asyncio.run(main())
