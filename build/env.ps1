# Copyright (c) 2023 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

function Set-BraveEnv() {
  param (
    $invocation,
    $customArgs
  )
  $scriptPath = $invocation.MyCommand.Path
  $isScriptSourced = $invocation.InvocationName -eq '.' -or $invocation.Line -eq ''

  # Do nothing if the script wasn't sourced.
  if (-not $isScriptSourced) {
    Write-Output "Please source the script: . $(Resolve-Path -Relative -LiteralPath $scriptPath) $customArgs"
    return
  }

  # Get script dir.
  $scriptDir = Split-Path -LiteralPath $scriptPath
  # Get environment variables to update.
  $genEnvOutput = npm run --silent --prefix "$scriptDir\.." gen_env

  # Set/unset environment variables. Vars to unset use `var=` syntax.
  foreach ($line in $genEnvOutput) {
    $envMatches = Select-String -InputObject $line -Pattern "^([^=]+)=(.*)$"
    if ($envMatches) {
      if ($customArgs[0] -eq "-v") {
        Write-Output $line
      }

      $key = $envMatches.Matches[0].Groups[1].Value
      $value = $envMatches.Matches[0].Groups[2].Value
      if (-not $value) {
        Remove-Item -Path Env:\$key -ErrorAction SilentlyContinue
      } else {
        Set-Item -Path Env:\$key -Value "$value"
      }
    }
  }
}

Set-BraveEnv -invocation $MyInvocation -customArgs $args
Remove-Item -Path Function:\Set-BraveEnv
