#!/usr/bin/env bash
# Generate ECDSA-P256 key material for n nodes (M2)
set -euo pipefail
N="${1:-5}"
OUT="${2:-keys}"
mkdir -p "$OUT/pub"
for i in $(seq 1 "$N"); do
  openssl ecparam -name prime256v1 -genkey -noout -out "$OUT/node${i}.pem"
  openssl ec -in "$OUT/node${i}.pem" -pubout -out "$OUT/pub/node${i}.pem"
done
echo "generated $N keypairs under $OUT/"
