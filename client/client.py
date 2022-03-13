import socket

#Send file
with open(text_file, 'rb') as fs: 
    #Using with, no file close is necessary, 
    #with automatically handles file close
    csFT.send(b'BEGIN')
    while True:
        data = fs.read(1024)
        print('Sending data', data.decode('utf-8'))
        csFT.send(data)
        print('Sent data', data.decode('utf-8'))
        if not data:
            print('Breaking from sending data')
            break
    csFT.send(b'ENDED') # I used the same size of the BEGIN token
    fs.close()