#!/usr/bin/env bash

OPENRVDAS_PATH="/opt/openrvdas" # change local path to the directory containing the rs485 UDP config
LOCAL_PATH="/home/sssg/openrvdas_support/config"

echo "Activating virtual environment"
if [[ -f "$OPENRVDAS_PATH/venv/bin/activate" ]]; then
    # shellcheck source=/dev/null
    source "$OPENRVDAS_PATH/venv/bin/activate"
else
    echo "Warning: virtualenv activate not found at $OPENRVDAS_PATH/venv/bin/activate" >&2
fi

echo "Starting data simulation on UDP ports 57001, 57002, 57003"
python "$OPENRVDAS_PATH/logger/utils/simulate_data.py" \
    --config "$LOCAL_PATH/simulate_rs485.yaml" \
    --record_format "{timestamp} {record}"

# Deactivate virtualenv if available
if declare -f deactivate >/dev/null 2>&1; then
    deactivate
fi