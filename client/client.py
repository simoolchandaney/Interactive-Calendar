
# client.py - Ian Havenaar, Simran Moolchandaney, Jacob Schwartz

import socket                   # Import socket module
import sys

s = socket.socket()             # Create a socket object
host = socket.gethostname()     # Get local machine name
port = 41999                    # Reserve a port for your service.

s.connect((host, port))
s.send(sys.argv[1:])

with open('received_file', 'wb') as f:
    print 'file opened'
    while True:
        print('receiving data...')
        data = s.recv(1024)
        print('data=%s', (data))
        if not data:
            break
        # write data to a file
        f.write(data)

f.close()
print('Successfully get the file')
s.close()
print('connection closed')