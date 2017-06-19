# PStreamer
PStreamer[1] is a P2P real-time content distribution platform.
It is built on top of the GRAPES library [2] which is a mandatory dependency.
You can compile the vanilla executable or the use the libpstreamer library to easily code your own streaming platform.

## Compilation
In order to compile you need the GRAPES library [3].
You can specify the GRAPES folder path using an environment variable.

To obtain both the PStreamer library and the executable just launch
``
$> make
``

To turn on all the debugging feature, set the DEBUG environment variable:
``
$> DEBUG=1 make
``

To turn on only the chunk and signalling debugging function use the CFLAGS:
``
$> CFLAGS="-DLOG_CHUNK -DLOG_SIGNAL" make
``

## Test
In the "test" folder are stored the test files. To run them and check code consistency run:
``
$> make tests
``

## Example
Here it is a simple example using the executable and the RTP chunkiser; you need an RTP streaming source we suppose it is streaming using the ports 4000,4001,4002,4003;
The following will start a PStreamer source peer, getting the RTP flow from the RTP source and serving it to the rest of the P2P net through port 3999
``
$> ./pstreamer -p 0 -c "iface=lo,port=3999,chunkiser=rtp,base=4000,addr=127.0.0.1,max_delay_ms=50"
``
The following will start a PStreamer peer, which gets the source peer flow from port 4999 and will re-distribute an RTP flow using the ports 5000,5001,5002,5003
``
$> ./pstreamer -p 3999 -c "iface=lo,port=4999,dechunkiser=rtp,base=5000,addr=127.0.0.1"
``

## References
[1] http://peerstreamer.org
[2] Abeni, Luca, et al. "Design and implementation of a generic library for P2P streaming." Proceedings of the 2010 ACM workshop on Advanced video streaming techniques for peer-to-peer networks and social networking. ACM, 2010
[3] https://ans.disi.unitn.it/redmine/grapes.git
