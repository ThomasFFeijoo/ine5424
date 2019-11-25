import socket
import time

HOST = "127.0.0.1"  # Standard loopback interface address (localhost)
PORT = 65432        # Port to listen on (non-privileged ports are > 1023)

SEND_DATA_TO_CLIENT = True


def send_data_to_client(conn):
    # TODO: gerar valores aleatórios para as mensagens NMEAs (4 âncoras que já possuem posição definida)
    conn.sendall('$GPGGA,183730,90,N,0,E,1,05,1.6,0,M,-24.1,M,,*75'.encode())
    #time.sleep(10)
    #conn.sendall('$GPGGA,666666,6666.555,S,44444.333,E,2,06,2.5,777.1,N,-74.2,N,,*86'.encode())

def parse(conn):
    #data = conn.recv(1024)
    #print("Message received: ", data.decode())

    if SEND_DATA_TO_CLIENT:
        send_data_to_client(conn)



with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.bind((HOST, PORT))
    while True:
        print(f"Waiting for connection in {HOST}:{PORT}")
        s.listen()
        conn, addr = s.accept()
        with conn:
            print("Connected by", addr)
            parse(conn)
            print("Connection Closed!")
