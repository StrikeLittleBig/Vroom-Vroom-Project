import socket, struct, time

ESP_IP = "192.168.4.1"  # IP du SoftAP
PORT   = 5555

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

def send_packet(buttons, lx, ly, rx, ry, lt, rt):
    pkt = struct.pack('<Iiiiiii', buttons, lx, ly, rx, ry, lt, rt)
    sock.sendto(pkt, (ESP_IP, PORT))

# Test: boutons = 0b00001111
send_packet(0x0F, 10, -10, 0, 0, 0, 0)

for _ in range(3):
    send_packet(0xA5A5A5A5, 10, 0, 0, 0, -13, 0)
    time.sleep(0.2)
