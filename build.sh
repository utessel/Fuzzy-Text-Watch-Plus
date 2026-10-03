#!/usr/bin/env bash
#
# Simple build script: sets up virtualenv, generates SDF font, and builds the watchface.
#

set -e

# Change to project root directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Ensure pebble and local tools are in PATH
export PATH="$HOME/.local/bin:$PATH"

# 1. Virtual Environment Setup
VENV_DIR=".venv"
if [ ! -d "$VENV_DIR" ]; then
    echo "==> Erstelle Python venv ($VENV_DIR)..."
    python3 -m venv "$VENV_DIR"
fi

# 2. Dependency Check (fonttools)
if ! "$VENV_DIR/bin/python3" -c "import fontTools" 2>/dev/null; then
    echo "==> Installiere fonttools in venv..."
    "$VENV_DIR/bin/pip" install --quiet fonttools
fi

# 3. Generate SDF Font
echo "==> Generiere SDF-Font aus TTF..."
"$VENV_DIR/bin/python3" SDFLib/ttf2sdf.py SDFLib/sdf_config.ini

# 4. Build Watchface
echo "==> Starte pebble build..."
pebble build "$@"
