"""Funções compartilhadas para mensagens binárias usando MessagePack."""
from datetime import datetime, timezone
import msgpack

def now():
    return datetime.now(timezone.utc).isoformat()

def pack(data):
    return msgpack.packb(data, use_bin_type=True)

def unpack(data):
    return msgpack.unpackb(data, raw=False)
