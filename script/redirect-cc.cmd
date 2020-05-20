@ECHO OFF
:: brave/script/redirect-cc.py is copied to %CD% and renamed to redirect.py in
:: the beginning of the build. See brave-browser/lib/util.js util.copyRedirectCC
python redirect.py %*
