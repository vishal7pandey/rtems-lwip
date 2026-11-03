#!/usr/bin/env bash
set -Eeuo pipefail

git config --global --add safe.directory "*" || true

# If RTEMS 7 sparc/leon3 BSP is not already installed in /opt/rtems/7, build and install it now
if [[ ! -f /opt/rtems/7/lib/pkgconfig/sparc-rtems7-leon3.pc ]]; then
  echo "RTEMS 7 sparc/leon3 BSP not found in /opt/rtems/7; building and installing from tc1/5776-base @ bfabfac..."
  RTEMS_SRC=$(mktemp -d /tmp/rtems-bsp.XXXXXX)
  git clone --depth 50 --branch tc1/5776-base https://github.com/vishal7pandey/rtems.git "${RTEMS_SRC}"
  pushd "${RTEMS_SRC}"
  git checkout bfabfac0ec1df928508074a7f4ed2d845b829417
  cat <<'EOF' > config.ini
[sparc/leon3]
BUILD_TESTS = False
EOF
  ./waf configure --prefix=/opt/rtems/7
  ./waf -j"$(nproc)"
  ./waf install
  popd
  rm -rf "${RTEMS_SRC}"
fi

git submodule update --init

./waf configure \
  --prefix=/opt/rtems/7 \
  --rtems-tools=/opt/rtems/7 \
  --rtems-bsps=sparc/leon3

if [[ "${CODEQL_BUILD:-0}" == "1" ]]; then
  ./waf -v -j1
else
  ./waf -j"$(nproc)"
fi

if [[ "${RUN_TESTS:-0}" == "1" ]]; then
  mkdir -p logs
  rtems-test \
    --log=logs/leon3-sis \
    --log-mode=all \
    --report-path=logs/leon3-sis-report \
    --report-format=json \
    --rtems-bsp=leon3-sis \
    build/sparc-rtems7-leon3/networking01.exe | tee logs/leon3-sis-console.log

  awk -F: '
    /^Passed:/ { gsub(/[[:space:]]/, "", $2); passed = $2 + 0 }
    /^(Failed|Timeout|Invalid|Test too long|Wrong Version|Wrong Build|Wrong Tools):/ {
      gsub(/[[:space:]]/, "", $2)
      if ($2 + 0 > 0) failed = 1
    }
    END {
      if (failed || passed < 1) {
        exit 1
      }
    }
  ' logs/leon3-sis-console.log || {
    echo "RTEMS SIS tests failed or no test passed (require Passed >= 1); see logs/leon3-sis-console.log" >&2
    exit 1
  }
fi
