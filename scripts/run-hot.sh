#!/usr/bin/env bash
set -e

APP="$HOME/deploy/lvglsim"
export LV_HIDE_CURSOR="${LV_HIDE_CURSOR:-1}"
export LV_LINUX_FBDEV_DEVICE="${LV_LINUX_FBDEV_DEVICE:-/dev/fb0}"
export LV_LINUX_EVDEV_POINTER_DEVICE="${LV_LINUX_EVDEV_POINTER_DEVICE:-/dev/input/event2}"

prev_mtime=""
pid=""

cleanup(){
  if [[ -n "$pid" ]] && kill -0 "$pid" 2>/dev/null; then
    kill -TERM "$pid" 2>/dev/null || true
    wait "$pid" 2>/dev/null || true
  fi
  exit 0
}
trap cleanup TERM INT

while true; do
  if [[ ! -x "$APP" ]]; then
    sleep 1; continue
  fi

  mtime=$(stat -c %Y "$APP" 2>/dev/null || echo 0)

  if [[ "$mtime" != "$prev_mtime" ]]; then
    if [[ -n "$pid" ]] && kill -0 "$pid" 2>/dev/null; then
      kill -TERM "$pid" 2>/dev/null || true
      wait "$pid" 2>/dev/null || true
    fi
    prev_mtime="$mtime"
    "$APP" & pid=$!
  else
    if ! kill -0 ${pid:-0} 2>/dev/null; then
      "$APP" & pid=$!
    fi
  fi
  sleep 1
done


