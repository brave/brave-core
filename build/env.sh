#!/usr/bin/env bash
# Copyright (c) 2023 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

brave_env::set_brave_env() {
  # Helper to check if the script is sourced or not.
  brave_env::is_script_sourced() {
    if [[ -n "$ZSH_VERSION" ]]; then
      case $ZSH_EVAL_CONTEXT in
      *:file:*)
        return 0
        ;;
      *)
        return 1 # NOT sourced.
        ;;
      esac
    else
      case ${0##*/} in
      env.sh)
        return 1 # NOT sourced.
        ;;
      *)
        return 0
        ;;
      esac
    fi
  }

  # Do nothing if the script wasn't sourced.
  if ! brave_env::is_script_sourced; then
    echo Please source the script: . $0 $@
    return
  fi
  unset -f brave_env::is_script_sourced

  # Get script dir. Supports bash and zsh syntax.
  local script_dir=$(dirname "${BASH_SOURCE[0]:-${(%):-%x}}")
  # Get environment variables to update.
  local gen_env_output=$(npm run --silent --prefix "$script_dir/.." gen_env)

  # Set/unset environment variables. Vars to unset use `var=` syntax.
  while IFS=$'\n' read -r line; do
    if [[ $line =~ ^([^=]+)=(.*)$ ]]; then
      if [[ "$1" == "-v" ]]; then
        echo $line
      fi

      # Extract regex matches. Supports bash and zsh syntax.
      local key=${BASH_REMATCH[1]:-${match[1]}}
      local val=${BASH_REMATCH[2]:-${match[2]}}
      if [[ -z "$val" ]]; then
        unset $key
      else
        if [[ "$key" =~ ^(PATH|PYTHONPATH)$ && "$(uname -s)" =~ ^(MINGW|CYGWIN) ]]; then
          val=$(cygpath -p "$val")
        fi
        export $key="$val"
      fi
    fi
  done <<<$gen_env_output
}

brave_env::set_brave_env "$@"
unset -f brave_env::set_brave_env
