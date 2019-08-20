#!/usr/bin/env python3

import socket
import time

HOST = '127.0.0.1'  # Standard loopback interface address (localhost)
PORT = 65432        # Port to listen on (non-privileged ports are > 1023)

def output(stream, fpath):
    # Header
    stream.sendall("HTTP/1.0 200 OK\nDate: {}\n".format(time.asctime()) )

    # Body
    with open(fpath) as fh:
        stream.sendall(fh.read())
    stream.close()
    

def main():
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.bind((HOST, PORT))
    s.listen()
    conn, addr = s.accept()
    with conn:
        print('Connected by', addr)
        while True:
            data = conn.recv(4096)
            print(data)

    # Parse request
    udata = data.decode('utf-8')
    arr = udata.split('r\n')
    path = arr[0].split()[1]
    if path == '/':
        path = '/index.html'
    path = os.path.join('www', path)
    output( conn, path )

    # Close server
    s.close()

if __name__ == '__main__':
    main()
