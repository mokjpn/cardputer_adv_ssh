#!/usr/bin/env bash
# Source this helper only in the configured cloud workspace.
export IDF_TOOLS_PATH=/workspace/.onboarding/idf-tools
export XDG_CACHE_HOME=/workspace/.cache
export PATH=/workspace/.onboarding/bootstrap/bin:$PATH
. /workspace/.onboarding/esp-idf/export.sh
