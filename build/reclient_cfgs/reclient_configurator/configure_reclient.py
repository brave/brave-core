#!/usr/bin/env python3
# Copyright (c) 2023 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import argparse
import functools
import glob
import inspect
import os
import re
import runpy
import shutil
import string
import subprocess
import sys


def main():
    args = parse_args()

    if not os.environ.get('RBE_service') and not args.force:
        # Do nothing if RBE environment is not configured.
        print('RBE_service environment variable is not set. '
              'Pass --force to configure reclient.')
        return

    Paths.init_from_args(args)
    ReclientConfigurator(args).configure()


def parse_args():
    parser = argparse.ArgumentParser(
        formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument(
        '--src_dir',
        required=True,
        help='Chromium src directory.',
        default=argparse.SUPPRESS,
    )
    parser.add_argument(
        '--exec_root',
        help=('Reclient exec_root directory. '
              'Should match \'rbe_exec_root\' GN arg.'),
        default=Paths.exec_root,
    )
    parser.add_argument(
        '--build_dir',
        help=('Build directory. Used to calculate relative paths, can be '
              'default if you build in any out/* directory.'),
        default=Paths.build_dir,
    )
    parser.add_argument(
        '--reclient_cfgs_dir',
        help=('Path to Chromium reclient_cfgs directory.'),
        default=Paths.reclient_cfgs_dir,
    )
    parser.add_argument(
        '--clang_base_path',
        help=('Chromium clang base path. '
              'Should match \'clang_base_path\' GN arg.'),
        default=Paths.clang_base_path,
    )
    parser.add_argument(
        '--linux_clang_base_path',
        help=('Directory to extract linux version of clang to run '
              'cross-compilation.'),
        default=Paths.linux_clang_base_path,
    )
    parser.add_argument(
        '--custom_py',
        help=('Path to python script to customize generated reclient configs.'),
    )
    parser.add_argument(
        '--force',
        help='Configure reclient even if RBE_service env var is not set.',
        action='store_true',
    )

    return parser.parse_args()


class ReclientConfigurator:
    args = None
    custom_py = None

    def __init__(self, args):
        self.args = args

    def configure(self):
        # Load custom py script to customize configs.
        self.load_custom_py()
        # Run custom py pre-configuration step.
        self.run_custom_py_pre_configure()

        # Linux clang toolchain and clang remote wrapper are required on
        # non-linux hosts to perform cross-compilation remotely.
        if not sys.platform.startswith('linux'):
            self.download_linux_clang_toolchain()
            self.generate_clang_remote_wrapper()

        # Reproxy config includes auth and network-related parameters.
        self.generate_reproxy_cfg()
        # Rewrapper configs describe how different tools should be run remotely.
        self.generate_rewrapper_cfgs()

        # Run custom py post-configuration step.
        self.run_custom_py_post_configure()

    def load_custom_py(self):
        if not Paths.custom_py:
            return

        custom_py_globals = dict(
            Paths=Paths,
            ReclientCfg=ReclientCfg,
            FileUtils=FileUtils,
            ShellTemplate=ShellTemplate,
        )
        self.custom_py = runpy.run_path(Paths.custom_py,
                                        init_globals=custom_py_globals)

    def run_custom_py_pre_configure(self):
        if self.custom_py and 'pre_configure' in self.custom_py:
            self.custom_py['pre_configure']()

    def run_custom_py_post_configure(self):
        if self.custom_py and 'post_configure' in self.custom_py:
            self.custom_py['post_configure']()

    @staticmethod
    def download_linux_clang_toolchain():
        subprocess.check_call([
            'python3',
            f'{Paths.src_dir}/tools/clang/scripts/update.py',
            '--output-dir',
            f'{Paths.linux_clang_base_path}',
            '--host-os',
            'linux',
        ])

    @staticmethod
    def generate_clang_remote_wrapper():
        if not os.path.exists(Paths.clang_base_path):
            raise RuntimeError(f'Cannot find {Paths.clang_base_path}.')

        # Load clang remote wrapper template.
        template_file = (f'{Paths.script_dir}/chromium-browser-clang/'
                         'clang_remote_wrapper.template')
        clang_remote_wrapper_template = FileUtils.read_text_file(template_file)

        # Find "include" directory inside clang installation. This directory
        # will be symlinked by remote wrapper for cross-compilation to work. The
        # path is clang-version dependent, so don't hardcode it.
        clang_include_dir_glob = glob.glob(
            f'{Paths.clang_base_path}/lib/**/include', recursive=True)
        if not clang_include_dir_glob:
            raise RuntimeError(
                f'Cannot find lib/**/include dir in {Paths.clang_base_path}. '
                f'If clang directory structure has changed, please update '
                f'{Paths.abspath(__file__)} and {template_file} if required.')
        clang_include_dir_abs = Paths.normpath(clang_include_dir_glob[0])
        assert os.path.isdir(clang_include_dir_abs), clang_include_dir_abs
        clang_include_dir = Paths.relpath(clang_include_dir_abs,
                                          Paths.build_dir)
        linux_clang_include_dir = Paths.relpath(
            clang_include_dir_abs.replace(Paths.clang_base_path,
                                          Paths.linux_clang_base_path),
            Paths.build_dir)

        # Variables to set in the template.
        template_vars = {
            'autogenerated_header': FileUtils.create_generated_header(
                template_file),
            'clang_base_path': Paths.relpath(Paths.clang_base_path,
                                             Paths.build_dir),
            'clang_include_dir': clang_include_dir,
            'linux_clang_base_path': Paths.relpath(Paths.linux_clang_base_path,
                                                   Paths.build_dir),
            'linux_clang_include_dir': linux_clang_include_dir,
        }

        # Substitute variables into the template.
        clang_remote_wrapper = ShellTemplate(
            clang_remote_wrapper_template).substitute(template_vars)

        # Write the clang remote wrapper.
        FileUtils.write_text_file(
            (f'{Paths.src_dir}/buildtools/reclient_cfgs/chromium-browser-clang/'
             'clang_remote_wrapper'), clang_remote_wrapper)

    def generate_reproxy_cfg(self):
        # Load Chromium config template and remove everything starting with $
        # symbol on each line.
        reproxy_cfg = ReclientCfg.parse_from_string(
            re.sub(r'^([^$]+)\$.*$',
                   r'\1',
                   FileUtils.read_text_file(
                       f'{Paths.reclient_cfgs_dir}/reproxy_cfg_templates/'
                       'reproxy.cfg.template'),
                   flags=re.MULTILINE))

        # Merge with our config.
        source_cfg_paths = [
            f'{Paths.script_dir}/reproxy.cfg',
        ]
        for source_cfg_path in source_cfg_paths:
            reproxy_cfg = ReclientCfg.merge_cfg(reproxy_cfg, source_cfg_path)

        # Use scandeps_server.
        depsscanner_address = (f'exec://{Paths.src_dir}/'
                               'buildtools/reclient/scandeps_server')
        if sys.platform.startswith('win'):
            depsscanner_address += '.exe'
        reproxy_cfg['depsscanner_address'] = depsscanner_address

        # Set values from supported RBE_ environment variables.
        SUPPORTED_REPROXY_ENV_VARS = (
            'RBE_service',
            'RBE_service_no_auth',
            'RBE_tls_client_auth_cert',
            'RBE_tls_client_auth_key',
            'RBE_use_application_default_credentials',
        )
        for env_var in SUPPORTED_REPROXY_ENV_VARS:
            value = os.environ.get(env_var)
            if value:
                reproxy_cfg[env_var[4:]] = value

        # Launch a custom merge step if it exists.
        if self.custom_py and 'merge_reproxy_cfg' in self.custom_py:
            reproxy_cfg = self.custom_py['merge_reproxy_cfg'](reproxy_cfg)
            source_cfg_paths.append(Paths.custom_py)

        # Write the final config to the expected location.
        ReclientCfg.write_to_file(f'{Paths.reclient_cfgs_dir}/reproxy.cfg',
                                  reproxy_cfg, source_cfg_paths)

    def generate_rewrapper_cfgs(self):
        # Generate chromium-browser-clang configs.
        self.generate_rewrapper_cfg('chromium-browser-clang', 'linux')
        self.generate_rewrapper_cfg('chromium-browser-clang', 'mac')
        self.generate_rewrapper_cfg('chromium-browser-clang', 'windows')

        # Generate python configs.
        self.generate_rewrapper_cfg('python', 'linux')
        self.generate_rewrapper_cfg('python', 'mac')
        self.generate_rewrapper_cfg('python', 'windows')

    def generate_rewrapper_cfg(self, tool, host_os):
        # Load Chromium config for linux remote.
        rewrapper_cfg = ReclientCfg.parse_from_file(
            f'{Paths.reclient_cfgs_dir}/linux/{tool}/'
            f'rewrapper_linux.cfg')

        # Merge with our configs.
        source_cfg_paths = [
            f'{Paths.script_dir}/{tool}/rewrapper_base.cfg',
            f'{Paths.script_dir}/{tool}/rewrapper_{host_os}.cfg',
        ]
        for source_cfg_path in source_cfg_paths:
            rewrapper_cfg = ReclientCfg.merge_cfg(rewrapper_cfg,
                                                  source_cfg_path)

        # Launch a custom merge step if it exists.
        if self.custom_py and 'merge_rewrapper_cfg' in self.custom_py:
            rewrapper_cfg = self.custom_py['merge_rewrapper_cfg'](rewrapper_cfg,
                                                                  tool, host_os)
            source_cfg_paths.append(Paths.custom_py)

        # Write the final config to the expected location.
        ReclientCfg.write_to_file(
            f'{Paths.reclient_cfgs_dir}/{tool}/rewrapper_{host_os}.cfg',
            rewrapper_cfg, source_cfg_paths)


class Paths:
    script_dir = ''
    src_dir = ''
    exec_root = '{src_dir}'
    build_dir = '{src_dir}/out/a'
    reclient_cfgs_dir = '{src_dir}/buildtools/reclient_cfgs'
    clang_base_path = '{src_dir}/third_party/llvm-build/Release+Asserts'
    linux_clang_base_path = '{clang_base_path}_linux'

    custom_py = ''

    _path_vars = {}

    @classmethod
    def init_from_args(cls, args):
        cls.script_dir = cls._create_path(os.path.dirname(__file__),
                                          'script_dir')
        cls.src_dir = cls._create_path(args.src_dir, 'src_dir')
        cls.exec_root = cls._create_path(args.exec_root or cls.exec_root,
                                         'exec_root')
        cls.build_dir = cls._create_path(args.build_dir or cls.build_dir,
                                         'build_dir')
        cls.reclient_cfgs_dir = cls._create_path(
            args.reclient_cfgs_dir or cls.reclient_cfgs_dir,
            'reclient_cfgs_dir')
        cls.clang_base_path = cls._create_path(
            args.clang_base_path or cls.clang_base_path, 'clang_base_path')
        cls.linux_clang_base_path = cls._create_path(
            args.linux_clang_base_path or cls.linux_clang_base_path,
            'linux_clang_base_path')

        if args.custom_py:
            cls.custom_py = cls._create_path(args.custom_py, 'custom_py')

        # Ensure some dirs are a part of exec_root.
        exec_root_included_dirs = (
            cls.src_dir,
            cls.build_dir,
            cls.reclient_cfgs_dir,
            cls.clang_base_path,
            cls.linux_clang_base_path,
        )
        for directory in exec_root_included_dirs:
            assert directory.startswith(
                cls.exec_root
            ), f'{directory} should be a part of {cls.exec_root}'

    @classmethod
    def _create_path(cls, path, path_var):
        path = cls.abspath(path.format(**cls._path_vars))
        if path_var:
            cls._path_vars[path_var] = path
        return path

    @classmethod
    def format(cls, path):
        return cls.normpath(path.format(**cls._path_vars))

    @classmethod
    def wspath(cls, path):
        assert os.path.isabs(path), f'{path} is not absolute'
        return f'//{cls.relpath(path, cls.src_dir)}'

    @classmethod
    def relpath(cls, a, b):
        return cls.normpath(os.path.relpath(a, b))

    @classmethod
    def abspath(cls, path):
        return cls.normpath(os.path.abspath(path))

    @classmethod
    def normpath(cls, path):
        return os.path.normpath(path).replace('\\', '/')

    @classmethod
    def deterministic_path(cls, path):
        assert path == cls.abspath(path), f'{path} != {cls.abspath(path)}'

        if path.startswith(cls.src_dir):
            return cls.wspath(path)

        if path.startswith(cls.script_dir):
            return (f'{{configurator_dir}}/'
                    f'{cls.relpath(path, cls.script_dir)}')

        return os.path.basename(path)


### Reclient config manipulation helpers. ###
class ReclientCfg:
    @classmethod
    def parse_from_file(cls, cfg_path):
        return dict(cls.enumerate_from_file(cfg_path))

    @classmethod
    def parse_from_string(cls, cfg_str):
        return dict(cls.parse_lines(cfg_str.split('\n')))

    @classmethod
    def write_to_file(cls, cfg_path, cfg, source_cfg_paths):
        assert isinstance(cfg, dict)
        cfg_to_write = FileUtils.create_generated_header(
            source_cfg_paths) + '\n\n'
        for key, value in cfg.items():
            formatted_value = cls.to_cfg_value(key, value)
            if formatted_value:
                cfg_to_write += f'{formatted_value}\n'

        FileUtils.write_text_file(cfg_path, cfg_to_write)

    @classmethod
    def enumerate_from_file(cls, cfg_path):
        with open(cfg_path, 'r') as f:
            yield from cls.parse_lines(f)

    @classmethod
    def merge_cfg(cls, reclient_cfg, cfg):
        if isinstance(cfg, dict):
            cfg_items = cfg.items()
        else:
            cfg_items = cls.enumerate_from_file(cfg)

        for key, value in cfg_items:
            reclient_cfg = cls.merge_cfg_item(reclient_cfg, {key: value})
        return reclient_cfg

    @classmethod
    def parse_lines(cls, cfg_lines):
        for cfg_line in cfg_lines:
            cfg_line = cfg_line.strip()
            if not re.match(r'^\w+=', cfg_line):
                continue
            key, value = cfg_line.split('=', 1)
            yield key, cls.from_cfg_value(key, value)

    @classmethod
    def from_cfg_value(cls, key, value):
        KEY_VALUE_PARAMS = {
            'labels',
            'platform',
        }
        LIST_PARAMS = {
            'env_var_allowlist',
            'input_list_paths',
            'inputs',
            'output_list_paths',
            'output_files',
            'output_directories',
            'toolchain_inputs',
        }

        if key in KEY_VALUE_PARAMS:
            ret_val = {}
            for sub_kv in value.split(','):
                if not sub_kv:
                    continue
                if '=' not in sub_kv:
                    raise RuntimeError(f'key=value expected for key: {key}')
                sub_key, sub_value = sub_kv.split('=', 1)
                ret_val[sub_key] = sub_value
            return ret_val

        if key in LIST_PARAMS:
            if not value:
                return []
            return value.split(',')

        return value

    @classmethod
    def to_cfg_value(cls, key, value, rebase_paths=True):
        if isinstance(value, dict):
            sub_keys_values = []
            for sub_key, sub_value in value.items():
                sub_keys_values.append(
                    cls.to_cfg_value(sub_key, sub_value, rebase_paths=False))
            return cls.to_cfg_value(key, sub_keys_values, rebase_paths=False)

        rebase_path_func = functools.partial(
            cls.rebase_if_path_value, key) if rebase_paths else lambda v: v

        if isinstance(value, list):
            value = ','.join(map(rebase_path_func, filter(None, value)))
            return cls.to_cfg_value(key, value, rebase_paths=False)

        return f'{key}={rebase_path_func(value)}' if value else None

    @classmethod
    def rebase_if_path_value(cls, key, value):
        # Describes relative to what each rewrapper parameter should be in the
        # config.
        PATHS_RELATIVE_TO = {
            'input_list_paths': Paths.exec_root,
            'inputs': Paths.exec_root,
            'output_files': Paths.exec_root,
            'output_list_paths': Paths.exec_root,
            'output_directories': Paths.exec_root,
            'toolchain_inputs': Paths.exec_root,
            'local_wrapper': Paths.build_dir,
            'remote_wrapper': Paths.build_dir,
        }

        relative_to = PATHS_RELATIVE_TO.get(key)
        if relative_to:
            value = Paths.format(value)
            if os.path.isabs(value):
                value = Paths.relpath(value, relative_to)

        return value

    @classmethod
    def merge_cfg_item(cls, a, b):
        if isinstance(a, dict):
            assert isinstance(b, dict)
            if not b:
                a.clear()
            else:
                for key in b:
                    if key in a:
                        a[key] = cls.merge_cfg_item(a[key], b[key])
                    else:
                        a[key] = b[key]
        elif isinstance(a, list):
            assert isinstance(b, list)
            if not b:
                a.clear()
            else:
                a.extend(b)
        else:
            a = b

        return a


class FileUtils:
    GENERATED_FILE_HEADER = inspect.cleandoc('''
    # AUTOGENERATED FILE - DO NOT EDIT
    # Generated by:
    {source_script}
    # To edit update:
    {source_files}
    # And rerun configurator.
    ''')

    @classmethod
    def read_text_file(cls, filepath):
        with open(filepath, 'r') as f:
            return f.read()

    @classmethod
    def write_text_file(cls, filepath, data_to_write):
        if os.path.isfile(filepath):
            with open(filepath, 'r') as f:
                if f.read() == data_to_write:
                    return

        os.makedirs(os.path.dirname(filepath), exist_ok=True)

        filepath_new = filepath + '.new'
        with open(filepath_new, 'w', newline='\n') as f:
            f.write(data_to_write)

        shutil.move(filepath_new, filepath)

    @classmethod
    def create_generated_header(cls, source_files):
        if not isinstance(source_files, (list, tuple)):
            source_files = (source_files, )

        script_file = Paths.deterministic_path(Paths.abspath(__file__))
        source_script = f'# {script_file}'
        source_files = '\n'.join(
            [f'# {Paths.deterministic_path(f)}' for f in source_files])

        return cls.GENERATED_FILE_HEADER.format(
            source_script=source_script,
            source_files=source_files,
        )


class ShellTemplate(string.Template):
    delimiter = '%'


if __name__ == '__main__':
    main()
