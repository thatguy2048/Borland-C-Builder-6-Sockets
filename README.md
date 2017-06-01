# Borland-C-Builder-6-Sockets
Implementation of TClientSocket and TServerSocket in borland c++ builder 6.  This is mostly a wrapper around the TClientSocket and TServerSocket classes, which and prevents attempting re-connects without destroying the socket, which is a known leak.
