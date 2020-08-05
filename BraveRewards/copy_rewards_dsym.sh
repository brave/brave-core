#!/bin/sh

#
# Copies the BraveRewards symbols file into the archive
#

REWARDS_DSYM_FILE="$SRCROOT/BraveRewards/BraveRewards.dSYM.zip"
[ -f $REWARDS_DSYM_FILE ] && unzip -oq "$REWARDS_DSYM_FILE" -d "$BUILT_PRODUCTS_DIR"
