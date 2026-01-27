import paho.mqtt.client as mqtt
import ssl
import time
import json

# ----- START OF CONFIGURATION -----
# IMPORTANT: Replace this with your actual value

AWS_IOT_ENDPOINT = "a1zx1lood4k4kc-ats.iot.ap-southeast-2.amazonaws.com"  # Find this in your AWS IoT Core -> Settings
CLIENT_ID = "flood_test_client"           # Can be any unique name

# Paths to your certificate files (relative to the 'data' directory)
# These filenames match the files detected in your project.
CA_CERTIFICATE_PATH = "ca.pem"
CLIENT_CERTIFICATE_PATH = "cert.crt"
PRIVATE_KEY_PATH = "private.key"

# The topic to flood
TARGET_TOPIC = "esp32/data"

# Number of messages to send
MESSAGE_COUNT = 500
# ----- END OF CONFIGURATION -----


def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connected to AWS IoT!")
        print(f"Attempting to flood topic '{TARGET_TOPIC}' with {MESSAGE_COUNT} messages...")
    else:
        print(f"Failed to connect, return code {rc}\n")

# --- Main script ---

# ** THE FIX IS ON THIS LINE **
# We now specify the callback API version required by paho-mqtt v2.0+
client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION1, CLIENT_ID)

client.on_connect = on_connect

# Configure TLS/SSL
client.tls_set(
    ca_certs=CA_CERTIFICATE_PATH,
    certfile=CLIENT_CERTIFICATE_PATH,
    keyfile=PRIVATE_KEY_PATH,
    tls_version=ssl.PROTOCOL_TLSv1_2
)

client.connect(AWS_IOT_ENDPOINT, 8883, 60)
client.loop_start() # Start a background thread to handle network traffic

# Wait a moment for the connection to establish
time.sleep(1)

# The MQTT Flood
for i in range(MESSAGE_COUNT):
    payload = {
        "moisture_percent": 50,
        "temperature_c": 25.0,
        "humidity_percent": 60.0,
        "watering_active": False,
        #"message_id": i + 1
    }
    
    # We use QoS 0 for a flood attack for max speed
    result = client.publish(TARGET_TOPIC, json.dumps(payload), qos=0)
    print(f"Message {i+1} sent... Status: {result.rc}")
    
    # If the publish call fails, the client is likely being throttled
    if result.rc != mqtt.MQTT_ERR_SUCCESS:
        print("\nPublish failed. The client is likely being throttled by AWS IoT.")
        print("This is the expected behavior in the 'before' state.")
        break
    
    time.sleep(0.01) # Small delay to avoid overwhelming the local client instantly

print("\nFlood test finished.")
client.loop_stop()
client.disconnect()