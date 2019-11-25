import socket
import time

HOST = "127.0.0.1"  # Standard loopback interface address (localhost)
PORT = 65432        # Port to listen on (non-privileged ports are > 1023)

SEND_DATA_TO_CLIENT = True


def send_data_to_client(conn):
    # TODO: gerar valores aleatórios para as mensagens NMEAs (4 âncoras que já possuem posição definida)
    conn.sendall('$GPGGA,183730,0,N,0,E,1,05,1.6,-6378037,M,-24.1,M,,*75'.encode()) # x = 100, y = 0, z = 0
    time.sleep(5)
    conn.sendall('$GPGGA,183730,90,N,0,E,1,05,1.6,-6356752.3,M,-24.1,M,,*75'.encode()) # x = 0, y = 0, z = 0

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
