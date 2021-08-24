import socket
import time 
import sys 
 
import argparse 
 
host = '192.168.1.245' 
 
def echo_client(port): 
    """ A simple echo client """ 
    # Create a TCP/IP socket 
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM) 
    # Connect the socket to the server 
    server_address = (host, port) 
    print ("Connecting to %s port %s" % server_address) 
    sock.connect(server_address) 
     
    # Send data 
    try: 
        # Send data 
        message = "GET /Go0.jpg HTTP/1.1\r\nHost: 192.168.1.20\r\n\r\n"
 
        print ("Sending %s" % message) 
        sock.sendall(message.encode('utf-8'))
        time.sleep(10)
        # Look for the response 
        
#        amount_received = 0 
#        amount_expected = len(message) 
#        while amount_received < amount_expected: 
#            data = sock.recv(128) 
#            amount_received += len(data) 
#            print ("Received: %s" % data) 
    except socket.error as e: 
        print ("Socket error: %s" %str(e)) 
    except Exception as e: 
        print ("Other exception: %s" %str(e)) 
    finally: 
        print ("Closing connection to the server") 
        sock.close() 
     
if __name__ == '__main__': 
    echo_client(80) 