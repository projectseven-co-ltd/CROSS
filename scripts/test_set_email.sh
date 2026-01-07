#!/usr/bin/env bash
set -euo pipefail

# Usage:
#   ./scripts/test_set_email.sh <device_base_url> [api_token] [email]
# Example:
#   ./scripts/test_set_email.sh http://jacob-f4e9d4.local "s3cr3t" "you@example.com"

DEVICE=${1:-http://localhost}
TOKEN=${2:-}
EMAIL=${3:-you@example.com}

post_email() {
  local url="$1"
  local token="$2"
  local email="$3"

  if [[ -n "$token" ]]; then
    curl -s -w "\n%{http_code}" -X POST "$url/api/set-email" \
      -H "Content-Type: application/json" \
      -H "Authorization: Bearer $token" \
      --data-raw "{\"email\":\"$email\"}"
  else
    curl -s -w "\n%{http_code}" -X POST "$url/api/set-email" \
      -H "Content-Type: application/json" \
      --data-raw "{\"email\":\"$email\"}"
  fi
}

echo "Posting email to $DEVICE (email=$EMAIL)"
resp=$(post_email "$DEVICE" "$TOKEN" "$EMAIL")
body=$(echo "$resp" | sed '$d')
code=$(echo "$resp" | tail -n1)

echo "Response code: $code"
echo "Body: $body"

if [[ "$code" != "200" ]]; then
  echo "POST failed"
  exit 2
fi

status=$(curl -s "$DEVICE/api/status")
echo "Status: $status"

echo "Done."