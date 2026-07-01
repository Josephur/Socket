#!/usr/bin/env bash
# Installs every dependency listed in libraries.txt via arduino-cli, so every
# developer machine and CI end up with identical library versions.
set -euo pipefail

cd "$(dirname "$0")/.."

while IFS= read -r line; do
  line="$(echo "$line" | sed 's/#.*//' | xargs)"
  [ -z "$line" ] && continue
  echo "Installing: $line"
  arduino-cli lib install "$line"
done < libraries.txt

# LVGL's internal config header search (lv_conf_internal.h) looks for
# lv_conf.h next to the lvgl library folder itself, not in the sketch -- so
# our pinned copy (src/lv_conf.h) has to be placed there after every install.
user_dir="$(arduino-cli config get directories.user)"
echo "Placing lv_conf.h in $user_dir/libraries/"
cp src/lv_conf.h "$user_dir/libraries/lv_conf.h"
