#!/usr/bin/env bash
# Throwaway local sshd for testing pubkey auth and write operations (put/mkdir/rm).
# Not a security boundary - dev/test use only, binds to 127.0.0.1:2222.
set -euo pipefail

FIXTURE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)/build/sshd-fixture"
mkdir -p "$FIXTURE_DIR"

HOST_KEY="$FIXTURE_DIR/host_key"
CLIENT_KEY="$FIXTURE_DIR/client_key"
AUTH_KEYS="$FIXTURE_DIR/authorized_keys"
SSHD_CONFIG="$FIXTURE_DIR/sshd_config"

[ -f "$HOST_KEY" ] || ssh-keygen -t ed25519 -f "$HOST_KEY" -N "" -q
[ -f "$CLIENT_KEY" ] || ssh-keygen -t ed25519 -f "$CLIENT_KEY" -N "" -q
ssh-keygen -y -f "$CLIENT_KEY" > "$AUTH_KEYS"

cat > "$SSHD_CONFIG" <<EOF
Port 2222
ListenAddress 127.0.0.1
HostKey "$HOST_KEY"
AuthorizedKeysFile "$AUTH_KEYS"
PasswordAuthentication no
PubkeyAuthentication yes
UsePAM no
Subsystem sftp internal-sftp
PidFile "$FIXTURE_DIR/sshd.pid"
EOF

case "${1:-start}" in
  start)
    /usr/bin/sshd -f "$SSHD_CONFIG" -D &
    echo $! > "$FIXTURE_DIR/sshd.pid"
    echo "sshd started on 127.0.0.1:2222 (pid $(cat "$FIXTURE_DIR/sshd.pid"))"
    echo "client key: $CLIENT_KEY"
    ;;
  stop)
    kill "$(cat "$FIXTURE_DIR/sshd.pid")" 2>/dev/null || true
    ;;
  *)
    echo "usage: $0 [start|stop]" >&2
    exit 1
    ;;
esac
