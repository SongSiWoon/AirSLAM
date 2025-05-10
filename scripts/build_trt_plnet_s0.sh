#!/usr/bin/env bash
set -euo pipefail

if [ $# -lt 2 ]; then
  echo "Usage: $0 <onnx_model.onnx> <output.engine> [workspace_mb]"
  exit 1
fi

MODEL_ONNX="$1"
ENGINE_OUT="$2"
WORKSPACE_MB="${3:-2048}"

TRTEXEC=/usr/src/tensorrt/bin/trtexec

# 빌드 옵션
ARGS=(
  --onnx="${MODEL_ONNX}"
  --saveEngine="${ENGINE_OUT}"
  --workspace="${WORKSPACE_MB}"
  --fp16
  --minShapes="input:1x1x100x100"
  --optShapes="input:1x1x512x512"
  --maxShapes="input:1x1x1500x1500"
  --skipInference
  --verbose
)

echo "=== trtexec ${ARGS[*]} ==="
"$TRTEXEC" "${ARGS[@]}"

echo "Stage1 engine built and saved to ${ENGINE_OUT}"

