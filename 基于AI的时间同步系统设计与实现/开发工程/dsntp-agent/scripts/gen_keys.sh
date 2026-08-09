# 生成 ECDSA-P256 测试密钥（需 openssl）
# 用法: ./scripts/gen_keys.sh 5 keys
set -e
N=${1:-5}
OUT=${2:-keys}
mkdir -p "$OUT"
for i in $(seq 1 "$N"); do
  openssl ecparam -name prime256v1 -genkey -noout -out "$OUT/node_${i}_priv.pem"
  openssl ec -in "$OUT/node_${i}_priv.pem" -pubout -out "$OUT/node_${i}_pub.pem"
done
echo "generated $N keypairs in $OUT"
