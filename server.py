import socket

HOST = "127.0.0.1"  # Standard loopback interface address (localhost)
PORT = 65432        # Port to listen on (non-privileged ports are > 1023)

SEND_DATA_TO_CLIENT = True

def send_data_to_client(conn):
    conn.sendall('100,0,0#'.encode())


def parse(conn):
    while True:
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