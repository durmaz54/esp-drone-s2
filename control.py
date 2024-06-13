import socket
import keyboard
import struct
import time

# Sunucu bilgileri
SERVER_IP = '192.168.4.1'  # Sunucu IP adresi
SERVER_PORT = 1234      # Sunucu port numarası

# Değişkenler
throttle = 1000  # Başlangıç değeri (1000-2000 aralığında)
yaw = 0          # Başlangıç değeri (-180 ile 180 aralığında)
roll = 0         # Başlangıç değeri (-45 ile 45 aralığında)
pitch = 0        # Başlangıç değeri (-45 ile 45 aralığında)

# TCP istemcisi oluştur
client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
client_socket.connect((SERVER_IP, SERVER_PORT))

def clamp(value, min_value, max_value):
    return max(min_value, min(value, max_value))

def send_data():
    global throttle, yaw, roll, pitch
    # Verileri paketle ve başlangıç/bitiş karakterlerini ekle
    data = struct.pack('!iiii', throttle, yaw, roll, pitch)
    packet = b'\x02' + data + b'\x03'
    client_socket.sendall(packet)
    print(f"Veriler gönderildi: Throttle={throttle}, Yaw={yaw}, Roll={roll}, Pitch={pitch}")

def check_and_send():
    global throttle, yaw, roll, pitch
    if keyboard.is_pressed('up'):
        throttle = clamp(throttle + 20 , 1000, 2000)
        send_data()
        time.sleep(0.1)
    elif keyboard.is_pressed('down'):
        throttle = clamp(throttle - 20, 1000, 2000)
        send_data()
        time.sleep(0.1)
    elif keyboard.is_pressed('right'):
        yaw = clamp(yaw + 1, -180, 180)
        send_data()
        time.sleep(0.1)
    elif keyboard.is_pressed('left'):
        yaw = clamp(yaw - 1, -180, 180)
        send_data()
        time.sleep(0.1)
    elif keyboard.is_pressed('d'):
        roll = clamp(roll + 1, -45, 45)
        send_data()
        time.sleep(0.1)
    elif keyboard.is_pressed('a'):
        roll = clamp(roll - 1, -45, 45)
        send_data()
        time.sleep(0.1)
    elif keyboard.is_pressed('w'):
        pitch = clamp(pitch + 1, -45, 45)
        send_data()
        time.sleep(0.1)
    elif keyboard.is_pressed('s'):
        pitch = clamp(pitch - 1, -45, 45)
        send_data()
        time.sleep(0.1)

# Ana döngü
try:
    while True:
        check_and_send()
except KeyboardInterrupt:
    print("Program sonlandırıldı.")
finally:
    client_socket.close()
