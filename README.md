# spi-networkserver
A service provider based TCP server (uses IPv6 based methods). Also has the basics of a data type based distributed data model.

The point of this is to boil down the basics of a network server to the point that all the service provider needs to worry about
is the reading and writing to the connection
Key points:
- a service provider based approach
- IPC to offer shutdown capability via pipes.
- IPv4/IPv6 compliant for the most part. 
- spawns a new thread for each connection
- adding the basics for a distrubuted data model.
