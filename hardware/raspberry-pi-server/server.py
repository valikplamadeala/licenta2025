#!/usr/bin/env python3
import asyncio
import logging
import aiosqlite
import datetime
import socket
from aiocoap import *
from aiocoap.resource import Resource, Site

# Setup logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger("CoAP_Server")

# Global device states
cooler_state = "off"
kitchen_state = "off"
living_state = "off"
bedroom_state = "off"

# IP address of the ESP32C2 client
COAP_CLIENT_IP = "192.168.4.1"

# Funcție pentru inițializarea bazei de date
async def init_db(db):
    async with db.execute("""
        CREATE TABLE IF NOT EXISTS sensors (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp DATETIME,
            sensor_type TEXT,
            value REAL,
            source TEXT
        )
    """):
        await db.commit()
    async with db.execute("""
        CREATE TABLE IF NOT EXISTS device_states (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp DATETIME,
            device TEXT,
            state TEXT
        )
    """):
        await db.commit()
    logger.info("Baza de date SQLite inițializată")

# Funcție pentru logarea datelor senzorilor
async def log_sensor_data(db, sensor_type, value, source):
    try:
        async with db.execute(
            "INSERT INTO sensors (timestamp, sensor_type, value, source) VALUES (?, ?, ?, ?)",
            (datetime.datetime.now(), sensor_type, float(value), source)
        ):
            await db.commit()
    except Exception as e:
        logger.error(f"Eroare la salvarea datelor senzorului {sensor_type}: {e}")

# Funcție pentru logarea stărilor dispozitivelor
async def log_device_state(db, device, state):
    try:
        async with db.execute(
            "INSERT INTO device_states (timestamp, device, state) VALUES (?, ?, ?)",
            (datetime.datetime.now(), device, state)
        ):
            await db.commit()
    except Exception as e:
        logger.error(f"Eroare la salvarea stării dispozitivului {device}: {e}")

# Resource pentru temperatura ESP32
class ESP32TemperatureResource(Resource):
    def __init__(self, db):
        super().__init__()
        self.temperature = "0"
        self.db = db

    async def render_get(self, request):
        return Message(payload=self.temperature.encode('utf-8'))

    async def render_put(self, request):
        self.temperature = request.payload.decode('utf-8')
        logger.info(f"Received ESP32 temperature: {self.temperature}°C")
        await log_sensor_data(self.db, "temperature", self.temperature, "esp32")
        return Message(code=CHANGED, payload=self.temperature.encode('utf-8'))

# Resource pentru umiditate ESP32
class ESP32HumidityResource(Resource):
    def __init__(self, db):
        super().__init__()
        self.humidity = "0"
        self.db = db

    async def render_get(self, request):
        return Message(payload=self.humidity.encode('utf-8'))

    async def render_put(self, request):
        self.humidity = request.payload.decode('utf-8')
        logger.info(f"Received ESP32 humidity: {self.humidity}%")
        await log_sensor_data(self.db, "humidity", self.humidity, "esp32")
        return Message(code=CHANGED, payload=self.humidity.encode('utf-8'))

# Resource pentru gaz ESP32
class ESP32GazResource(Resource):
    def __init__(self, db):
        super().__init__()
        self.gaz = "0"
        self.db = db

    async def render_get(self, request):
        return Message(payload=self.gaz.encode('utf-8'))

    async def render_put(self, request):
        self.gaz = request.payload.decode('utf-8')
        logger.info(f"Received ESP32 gas level: {self.gaz}")
        await log_sensor_data(self.db, "gas", self.gaz, "esp32")
        return Message(code=CHANGED, payload=self.gaz.encode('utf-8'))

# Resource pentru alerte gaz
class GasAlertResource(Resource):
    async def render_put(self, request):
        alert_msg = request.payload.decode('utf-8')
        logger.warning(f"GAS ALERT: {alert_msg}")
        return Message(code=CHANGED, payload=alert_msg.encode('utf-8'))

# Resource pentru control lumină bucătărie
class KitchenLightResource(Resource):
    def __init__(self, db):
        super().__init__()
        self.db = db

    async def render_get(self, request):
        global kitchen_state
        return Message(payload=kitchen_state.encode('utf-8'))

    async def render_put(self, request):
        global kitchen_state
        kitchen_state = request.payload.decode('utf-8')
        logger.info(f"Kitchen light state updated to: {kitchen_state}")
        await log_device_state(self.db, "kitchen", kitchen_state)
        try:
            await send_command_to_esp32("kitchen", kitchen_state)
        except Exception as e:
            logger.error(f"Failed to send kitchen command to ESP32: {e}")
        return Message(code=CHANGED, payload=kitchen_state.encode('utf-8'))

# Resource pentru control lumină living
class LivingRoomLightResource(Resource):
    def __init__(self, db):
        super().__init__()
        self.db = db

    async def render_get(self, request):
        global living_state
        return Message(payload=living_state.encode('utf-8'))

    async def render_put(self, request):
        global living_state
        living_state = request.payload.decode('utf-8')
        logger.info(f"Living room light state updated to: {living_state}")
        await log_device_state(self.db, "living", living_state)
        try:
            await send_command_to_esp32("living", living_state)
        except Exception as e:
            logger.error(f"Failed to send living room command to ESP32: {e}")
        return Message(code=CHANGED, payload=living_state.encode('utf-8'))

# Resource pentru control lumină dormitor
class BedroomLightResource(Resource):
    def __init__(self, db):
        super().__init__()
        self.db = db

    async def render_get(self, request):
        global bedroom_state
        return Message(payload=bedroom_state.encode('utf-8'))

    async def render_put(self, request):
        global bedroom_state
        bedroom_state = request.payload.decode('utf-8')
        logger.info(f"Bedroom light state updated to: {bedroom_state}")
        await log_device_state(self.db, "bedroom", bedroom_state)
        try:
            await send_command_to_esp32("bedroom", bedroom_state)
        except Exception as e:
            logger.error(f"Failed to send bedroom command to ESP32: {e}")
        return Message(code=CHANGED, payload=bedroom_state.encode('utf-8'))

# Resource pentru control cooler
class CoolerResource(Resource):
    def __init__(self, db):
        super().__init__()
        self.db = db

    async def render_get(self, request):
        global cooler_state
        return Message(payload=cooler_state.encode('utf-8'))

    async def render_put(self, request):
        global cooler_state
        cooler_state = request.payload.decode('utf-8')
        logger.info(f"Cooler state updated to: {cooler_state}")
        await log_device_state(self.db, "cooler", cooler_state)
        try:
            await send_command_to_esp32("cooler", cooler_state)
        except Exception as e:
            logger.error(f"Failed to send cooler command to ESP32: {e}")
        return Message(code=CHANGED, payload=cooler_state.encode('utf-8'))

# Funcție pentru trimiterea comenzilor către ESP32
async def send_command_to_esp32(device, state):
    logger.info(f"Sending {state} command to ESP32 device: {device}")
    protocol = await Context.create_client_context()
    request = Message(code=Code.PUT, 
                     uri=f"coap://{COAP_CLIENT_IP}/{device}", 
                     payload=state.encode('utf-8'))
    try:
        response = await protocol.request(request).response
        logger.info(f"ESP32 response: {response.payload.decode('utf-8')}")
    except Exception as e:
        logger.error(f"Error sending command to ESP32: {e}")
    finally:
        await protocol.shutdown()

# Funcție principală
async def main():
    # Inițializare bază de date
    db = await aiosqlite.connect("smart_home.db")
    await init_db(db)

    # Creare arbore de resurse
    root = Site()
    root.add_resource(['esp32_temperature'], ESP32TemperatureResource(db))
    root.add_resource(['esp32_humidity'], ESP32HumidityResource(db))
    root.add_resource(['esp32_gaz'], ESP32GazResource(db))
    root.add_resource(['esp32_gas_alert'], GasAlertResource())
    root.add_resource(['kitchen'], KitchenLightResource(db))
    root.add_resource(['living'], LivingRoomLightResource(db))
    root.add_resource(['bedroom'], BedroomLightResource(db))
    root.add_resource(['cooler'], CoolerResource(db))

    # IP local (evită INADDR_ANY)
    local_ip = socket.gethostbyname(socket.gethostname())
    bind_address = (local_ip, 5683)

    await Context.create_server_context(root, bind=bind_address)
    logger.info("🚀 CoAP Server running")
    logger.info(f"Listening on: coap://{local_ip}:5683")
    logger.info(f"Communicating with ESP32 at: {COAP_CLIENT_IP}")

    await asyncio.get_running_loop().create_future()
    await db.close()

# Pornirea serverului
if __name__ == "__main__":
    asyncio.run(main())