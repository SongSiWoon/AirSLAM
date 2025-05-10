#!/usr/bin/env bash
set -euo pipefail

MODEL_ONNX="$1"
ENGINE_OUT="$2"
WORKSPACE_MB="${3:-2048}"    # 기본 2 GB
DLACORE="${4:-}"             # DLA 코어 번호 (없으면 GPU만 사용)

# 동적 텐서 프로파일(필요에 맞게 조정)
SHAPES=(
  "--minShapes=juncs_pred:1x2,lines_pred:1x4,idx_lines_for_junctions:1x2,inverse:1x1,iskeep_index:1x1,loi_features:1x16x16x16,loi_features_thin:1x4x16x16,loi_features_aux:1x4x16x16"
  "--optShapes=juncs_pred:250x2,lines_pred:20000x4,idx_lines_for_junctions:19998x2,inverse:20000x1,iskeep_index:20000x1,loi_features:1x128x128x128,loi_features_thin:1x4x128x128,loi_features_aux:1x4x128x128"
  "--maxShapes=juncs_pred:500x2,lines_pred:50000x4,idx_lines_for_junctions:50000x2,inverse:50000x1,iskeep_index:50000x1,loi_features:1x256x256x256,loi_features_thin:1x4x512x512,loi_features_aux:1x4x512x512"
)

# Jetson
SHAPES=(
  "--minShapes=juncs_pred:1x2,lines_pred:1x4,idx_lines_for_junctions:1x2,inverse:1x1,iskeep_index:1x1,loi_features:1x16x16x16,loi_features_thin:1x4x16x16,loi_features_aux:1x4x16x16"
  "--optShapes=juncs_pred:250x2,lines_pred:20000x4,idx_lines_for_junctions:20000x2,inverse:20000x1,iskeep_index:20000x1,loi_features:1x128x128x128,loi_features_thin:1x4x128x128,loi_features_aux:1x4x128x128"
  "--maxShapes=juncs_pred:500x2,lines_pred:50000x4,idx_lines_for_junctions:40000x2,inverse:40000x1,iskeep_index:40000x1,loi_features:1x256x256x256,loi_features_thin:1x4x512x512,loi_features_aux:1x4x512x512"
)


# trtexec 실행 경로 (Jetson의 실제 위치를 하나 골라주세요)
TRTEXEC=$(which trtexec || true)
if [[ -z "$TRTEXEC" ]]; then
  echo "Error: 'trtexec' not found in PATH. Please check installation."
  exit 1
fi


ARGS=(
  --onnx="$MODEL_ONNX"
  --saveEngine="$ENGINE_OUT"
  --fp16               # FP16 모드
  --skipInference
  --verbose
  --builderOptimizationLevel=1
  "${SHAPES[@]}"
)

# DLA 사용 옵션 (필요 없으면 지우세요)
if [[ -n "$DLACORE" ]]; then
  ARGS+=(--useDLACore="$DLACORE")
fi

echo "=== trtexec ==="
echo "$TRTEXEC ${ARGS[*]}"
"$TRTEXEC" "${ARGS[@]}"


