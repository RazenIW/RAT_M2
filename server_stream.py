import json
import socket
import threading
import requests
import base64
import os
from PIL import Image, ImageDraw, ImageFont
from io import BytesIO
import datetime
import time

PORT = 110

APIKEY = "Yq5GYNwzLcZVe9iHiWEbQVVtv6W44hGZvsCUVXVWmZkh6Mru8qFTEToVndDkZ5LH"

clients = {}


def text_on_img(image, text, pos_y):
    draw = ImageDraw.Draw(image)
    font = ImageFont.truetype("arial.ttf", 16)

    left, _, right, _ = font.getbbox(text)
    draw.text(((image.width - (right - left)) / 2, pos_y), text, fill="white", font=font, stroke_width=2, stroke_fill="black")

    return image


class UnknownHostException(Exception):
    pass


def close_socket(sock):
    sock.shutdown(socket.SHUT_RDWR)
    sock.close()


def receive_from_client(c, faddr, host):
    global clients

    while True:
        try:
            size = c.recv(4)

            if not size:
                raise socket.error

            size = int.from_bytes(size, "big")

            cursor_x = c.recv(2)
            cursor_x = int.from_bytes(cursor_x, "big")

            cursor_y = c.recv(2)
            cursor_y = int.from_bytes(cursor_y, "big")

            image = bytearray()

            while len(image) < size:
                image += c.recv(1024)

            image = image.decode("utf-8")
            image = base64.b64decode(image)

            image_edit = Image.open(BytesIO(image))

            if cursor_x != 0 and cursor_y != 0:
                cursor = Image.open("cursor.png")
                image_edit.paste(cursor, (cursor_x, cursor_y), cursor)

            addr = faddr.split(':')[0]
            date = datetime.datetime.now().strftime("%d/%m/%Y %H:%M:%S")

            image_edit = text_on_img(image_edit, f"{host} @ {addr}", 10)
            image_edit = text_on_img(image_edit, date, 35)

            image_edit.save(f"./images/{host}@{addr}.jpeg")

            print(f"[{date}] Saved screenshot from {host}, size : {size} bytes")

        except UnicodeDecodeError:
            print("Unable to decode image.")

        except socket.error:
            print(f"Disconnected {host} @ {faddr}")
            clients.pop(faddr)
            break


def accept_clients(server):
    global clients

    while True:
        c, addr = server.accept()
        faddr = ":".join([str(e) for e in addr])

        try:
            host = c.recv(1024).decode("utf-8")
            h = json.loads(host)

            if h["host"] is None:
                raise UnknownHostException

            host = h["host"]

            time.sleep(5)

            response = requests.get(f"http://localhost/webapi.php?key={APIKEY}&ipv4={addr[0]}&host={host}")
            d = json.loads(response.text)

            if d["status"] == "error":
                raise UnknownHostException

            print(f"Accepted {host} @ {faddr}")

            c.send("OK\0".encode("utf-8"))

            clients[faddr] = (d, c, threading.Thread(target=receive_from_client, args=(c, faddr, host)))
            clients[faddr][2].start()

        except (json.decoder.JSONDecodeError, UnicodeDecodeError, UnknownHostException):
            print(f"Refused {faddr} (json.decoder.JSONDecodeError)")
            c.send("ERR\0".encode("utf-8"))
            close_socket(c)


if __name__ == "__main__":
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    s.bind(("", PORT))
    s.listen(5)

    t = threading.Thread(target=accept_clients, args=(s,))
    t.daemon = True
    t.start()

    print(f"[+] Server started on port {PORT}.")

    while True:
        pass
