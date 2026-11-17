import zmq
ctx = zmq.Context()
xsub = ctx.socket(zmq.XSUB)
xpub = ctx.socket(zmq.XPUB)
xsub.bind("tcp://0.0.0.0:5557")
xpub.bind("tcp://0.0.0.0:5558")
print("[proxy] Pub/Sub pronto: XSUB 5557, XPUB 5558", flush=True)
try:
    zmq.proxy(xsub, xpub)
finally:
    xsub.close()
    xpub.close()
    ctx.term()
