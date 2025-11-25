import serial
import time
import paho.mqtt.client as mqtt
import sys
import json
import select

print("=== CoreIoT Gateway Started ===")

# --- Config ---
BROKER_ADDRESS = "app.coreiot.io"
PORT = 1883
ACCESS_TOKEN = "tranchianzero"
ACCESS_USERNAME = "tranchianzero"

SERIAL_PORT = '/dev/ttyACM0' 
SERIAL_BAUD = 115200

ser = None 

# --- MQTT ---
def subscribed(client, userdata, mid, granted_qos):
    print("Subscribed to RPC requests...")
def recv_message(client, userdata, message):
    global ser
    print("Received RPC: ", message.payload.decode("utf-8"))
    
    try:
        jsonobj = json.loads(message.payload)
        method = jsonobj.get('method')
        params = jsonobj.get('params')

        if ser and ser.is_open:
            
            # --- MODE 1: Pause/Resume colect sensor data ---
            if method == "setPause":
                if params == True:
                    print("-> Sending: PAUSE")
                    ser.write(b"PAUSE\n")
                else:
                    print("-> Sending: RESUME")
                    ser.write(b"RESUME\n")
                client.publish('v1/devices/me/attributes', json.dumps({'setPause': params}), 1)

            # --- MODE 2: Sleep (Input seconds) ---
            elif method == "setTimerSleep":
                try:
                    seconds = int(params)
                    if seconds > 0:
                        cmd = f"SLEEP:{seconds}\n"
                        print(f"-> Sending: {cmd.strip()}")
                        ser.write(cmd.encode('utf-8'))
                    else:
                        print("Error: Sleep time must be > 0")
                except ValueError:
                    print("Error: Invalid time format")

    except Exception as e:
        print(f"Error processing RPC: {e}")

def connected(client, usedata, flags, rc):
    if rc == 0:
        print("MQTT Connected successfully!!")
        client.subscribe("v1/devices/me/rpc/request/+")
    else:
        print(f"MQTT Connection failed with code {rc}")

def on_disconnect(client, userdata, rc):
    if rc != 0:
        print("MQTT Disconnected unexpectedly. Trying to reconnect...")

# --- START MQTT CLIENT ---
client = mqtt.Client("IOT")
client.username_pw_set(ACCESS_USERNAME, ACCESS_TOKEN)
client.on_connect = connected
client.on_disconnect = on_disconnect 
client.on_subscribe = subscribed
client.on_message = recv_message

# --- 1. CONNECT TO SERIAL ---
while ser is None:
    i, o, e = select.select([sys.stdin], [], [], 0)
    try:
        print(f"Connecting to Serial Port {SERIAL_PORT}...")
        ser = serial.Serial(SERIAL_PORT, SERIAL_BAUD, timeout=1)
        ser.flush()
        print("Serial connected!")
    except Exception as e:
        print(f"Serial error: {e}. Retrying in 5s...")
        time.sleep(5)

# --- 2. CONNECT TO MQTT ---
mqtt_connected = False
print(f"Connecting to MQTT Broker: {BROKER_ADDRESS}...")

while not mqtt_connected:
    i, o, e = select.select([sys.stdin], [], [], 0)
    try:
        client.connect(BROKER_ADDRESS, PORT)
        client.loop_start() 
        mqtt_connected = True
        time.sleep(2) 
    except Exception as e:
        print(f"Cannot connect to MQTT: {e}. Retrying in 5s...")
        time.sleep(5)

# --- 3. MAIN LOOP ---
print("Gateway is running... ")

try:
    while True:
        # READ DATA FROM ESP32
        if ser.in_waiting > 0:
            try:
                line = ser.readline().decode('utf-8').rstrip()
                if line.startswith("CMD:"):
                    print(f"ESP32 Status: {line}")
                elif ":" in line:
                    parts = line.split(":")
                    key = parts[0].strip()
                    value = parts[1].strip()
                    try:
                        value_float = float(value)
                        if key == "temp": key = "temperature"
                        elif key == "hum": key = "humidity"

                        collect_data = { key: value_float }
                        print(f"-> Uploading: {json.dumps(collect_data)}")
                        client.publish('v1/devices/me/telemetry', json.dumps(collect_data), 1)
                    except ValueError:
                         print(f"From ESP32 (Non-data): {line}")
                else:
                    print(f"From ESP32: {line}")

            except Exception as e:
                print(f"Serial parsing error: {e}")

        time.sleep(0.1)

except KeyboardInterrupt:
    print("\nStopping via Ctrl+C...")
finally:
    if ser and ser.is_open:
        ser.close()
    client.loop_stop()
    client.disconnect()
    print("Gateway stopped.")