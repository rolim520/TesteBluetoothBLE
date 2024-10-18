import asyncio
from bleak import BleakClient, BleakScanner
import json
import time

# UUIDs must match those in the ESP32 code
SERVICE_UUID = "12345678-1234-1234-1234-123456789012"
CHARACTERISTIC_UUID_TX = "12345678-1234-1234-1234-123456789013"  # ESP32 -> Computer
CHARACTERISTIC_UUID_RX = "12345678-1234-1234-1234-123456789014"  # Computer -> ESP32

# Function to handle received notifications (BLE characteristic changes)
def notification_handler(sender, data):
    # print(f"Received data from ESP32: {data.decode('utf-8')}")
    json_data = json.loads(data.decode('utf-8'))
    print("Parsed JSON:", json_data)

async def send_json_to_esp(client):
    """
    Function to send JSON data continuously to the ESP32.
    """
    number = 0
    while True:
        # Create JSON data to send
        json_data = {
            "message": "Hello from Python",
            "number": number
        }
        json_string = json.dumps(json_data)

        # Send the JSON data to the ESP32 via the RX characteristic
        await client.write_gatt_char(CHARACTERISTIC_UUID_RX, json_string.encode('utf-8'))
        # print(f"Sent JSON data to ESP32: {json_string}")

        number += 1
        await asyncio.sleep(1)  # Send data every 2 seconds

async def run_ble_client():
    # Scan for devices
    devices = await BleakScanner.discover()
    esp32_device = None

    # Find the ESP32 by its advertised name
    for device in devices:
        if device.name == "ESP32_JSON_BLE":
            esp32_device = device
            break

    if esp32_device is None:
        print("ESP32 not found")
        return

    # Connect to ESP32
    async with BleakClient(esp32_device.address) as client:
        print(f"Connected to {esp32_device.name}")

        # Enable notifications to receive data from ESP32
        await client.start_notify(CHARACTERISTIC_UUID_TX, notification_handler)

        # Start sending data to the ESP32 in parallel with listening for notifications
        await send_json_to_esp(client)

        # Keep the connection open indefinitely
        await asyncio.Future()  # Keep the event loop running

# Run the BLE client
loop = asyncio.get_event_loop()
loop.run_until_complete(run_ble_client())
