#!/usr/bin/env python3
import asyncio
from aiohttp import web
from aiocoap import Context, Message, Code
import aiosqlite
import logging

# Setup logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger("HTTP_Proxy")

# Funcție pentru a obține datele dintr-o resursă CoAP
async def get_coap_data(resource_path):
    protocol = await Context.create_client_context()
    request = Message(code=Code.GET, uri=f"coap://localhost/{resource_path}")
    try:
        response = await protocol.request(request).response
        return response.payload.decode('utf-8')
    except Exception as e:
        logger.error(f"Eroare la obținerea datelor CoAP pentru {resource_path}: {e}")
        return "0"
    finally:
        await protocol.shutdown()



async def get_sensor_stats(request):
    sensor_type = request.match_info.get('sensor')
    interval = request.rel_url.query.get('interval', 'day')  # 'day', 'week', 'month'
    if sensor_type not in ['temperature', 'humidity', 'gas']:
        return web.json_response({"error": "Tip de senzor invalid"}, status=400)

    if interval == "day":
        group_by = "date(timestamp)"
        label = "zi"
        limit = 7
    elif interval == "week":
        group_by = "strftime('%Y-%W', timestamp)"
        label = "saptamana"
        limit = 6
    elif interval == "month":
        group_by = "strftime('%Y-%m', timestamp)"
        label = "luna"
        limit = 6
    else:
        return web.json_response({"error": "Interval invalid"}, status=400)

    query = f"""
        SELECT {group_by} as perioada, 
               ROUND(AVG(value),2) as avg, 
               MAX(value) as max, 
               MIN(value) as min
        FROM sensors
        WHERE sensor_type = ?
        GROUP BY perioada
        ORDER BY perioada DESC
        LIMIT {limit}
    """
    async with aiosqlite.connect("smart_home.db") as db:
        cursor = await db.execute(query, (sensor_type,))
        rows = await cursor.fetchall()
        return web.json_response({
            "stats": [
                {"label": row[0], "avg": row[1], "max": row[2], "min": row[3]}
                for row in rows
            ]
        })


# =========================
# 2. USAGE COOLER + COST
# =========================

async def get_cooler_usage(request):
    interval = request.rel_url.query.get('interval', 'day')
    power_watt = 40        # Watt (schimbi dacă ai alt consum)
    cost_per_kwh = 1.5     # lei/kWh (modifici dacă ai alt tarif)

    if interval == "day":
        group = "date(timestamp)"
        limit = 7
    elif interval == "week":
        group = "strftime('%Y-%W', timestamp)"
        limit = 6
    elif interval == "month":
        group = "strftime('%Y-%m', timestamp)"
        limit = 6
    else:
        return web.json_response({"error": "Interval invalid"}, status=400)

    query = f"""
    WITH periods AS (
        SELECT {group} as perioada,
               timestamp,
               state
        FROM device_states
        WHERE device='cooler'
        ORDER BY timestamp
    )
    SELECT perioada,
           SUM(
               CASE WHEN state='on' THEN 
                    (JULIANDAY(LEAD(timestamp,1,timestamp) OVER (PARTITION BY perioada ORDER BY timestamp)) - JULIANDAY(timestamp))
               ELSE 0 END
           ) * 24 as ore_pornit
    FROM periods
    GROUP BY perioada
    ORDER BY perioada DESC
    LIMIT {limit}
    """
    async with aiosqlite.connect("smart_home.db") as db:
        cursor = await db.execute(query)
        rows = await cursor.fetchall()
        results = []
        for row in rows:
            ore = float(row[1] or 0)
            kwh = ore * (power_watt / 1000)
            cost = round(kwh * cost_per_kwh, 2)
            results.append({"label": row[0], "hours": round(ore,2), "cost": cost})
        return web.json_response({"usage": results})


# Funcție pentru a trimite comanda PUT către o resursă CoAP
async def put_coap_data(resource_path, payload):
    protocol = await Context.create_client_context()
    request = Message(code=Code.PUT, uri=f"coap://localhost/{resource_path}", payload=payload.encode('utf-8'))
    try:
        response = await protocol.request(request).response
        return response.payload.decode('utf-8')
    except Exception as e:
        logger.error(f"Eroare la trimiterea comenzii CoAP pentru {resource_path}: {e}")
        return "error"
    finally:
        await protocol.shutdown()

# Endpoint pentru istoricul temperaturii
async def get_temperature_history(request):
    async with aiosqlite.connect("smart_home.db") as db:
        cursor = await db.execute(
            "SELECT timestamp, value FROM sensors WHERE sensor_type = 'temperature' ORDER BY timestamp DESC LIMIT 100"
        )
        rows = await cursor.fetchall()
        return web.json_response({"history": [{"timestamp": str(row[0]), "value": row[1]} for row in rows]})

# Endpoint pentru istoricul umidității
async def get_humidity_history(request):
    async with aiosqlite.connect("smart_home.db") as db:
        cursor = await db.execute(
            "SELECT timestamp, value FROM sensors WHERE sensor_type = 'humidity' ORDER BY timestamp DESC LIMIT 100"
        )
        rows = await cursor.fetchall()
        return web.json_response({"history": [{"timestamp": str(row[0]), "value": row[1]} for row in rows]})

# Endpoint pentru istoricul gazului
async def get_gaz_history(request):
    async with aiosqlite.connect("smart_home.db") as db:
        cursor = await db.execute(
            "SELECT timestamp, value FROM sensors WHERE sensor_type = 'gas' ORDER BY timestamp DESC LIMIT 100"
        )
        rows = await cursor.fetchall()
        return web.json_response({"history": [{"timestamp": str(row[0]), "value": row[1]} for row in rows]})

# Rute HTTP pentru senzori
async def get_temperature(request):
    data = await get_coap_data("esp32_temperature")
    return web.json_response({"temperature": data})

async def get_humidity(request):
    data = await get_coap_data("esp32_humidity")
    return web.json_response({"humidity": data})

async def get_gaz(request):
    data = await get_coap_data("esp32_gaz")
    return web.json_response({"gaz": data})

# Rute HTTP pentru dispozitive smart home
async def get_device_state(request):
    device = request.match_info.get('device')
    if device not in ['kitchen', 'living', 'bedroom', 'cooler']:
        return web.json_response({"error": "Dispozitiv invalidRey"}, status=400)
    
    data = await get_coap_data(device)
    return web.json_response({device: data})

async def set_device_state(request):
    device = request.match_info.get('device')
    if device not in ['kitchen', 'living', 'bedroom', 'cooler']:
        return web.json_response({"error": "Dispozitiv invalid"}, status=400)
    
    try:
        data = await request.json()
        state = data.get("state")
        if state not in ["on", "off"]:
            return web.json_response({"error": "Stare invalidă"}, status=400)
        
        response = await put_coap_data(device, state)
        logger.info(f"Trimis comandă {state} pentru {device}, răspuns: {response}")
        return web.json_response({device: response})
    except Exception as e:
        logger.error(f"Eroare la procesarea cererii pentru {device}: {e}")
        return web.json_response({"error": str(e)}, status=500)

# Rută pentru a obține toate datele despre casă
async def get_smart_home_status(request):
    try:
        # Obține datele de la toți senzorii
        esp32_temp = await get_coap_data("esp32_temperature")
        esp32_humidity = await get_coap_data("esp32_humidity")
        esp32_gaz = await get_coap_data("esp32_gaz")
        
        # Obține starea dispozitivelor
        kitchen = await get_coap_data("kitchen")
        living = await get_coap_data("living")
        bedroom = await get_coap_data("bedroom")
        cooler = await get_coap_data("cooler")
        
        return web.json_response({
            "sensors": {
                "esp32_temperature": esp32_temp,
                "esp32_humidity": esp32_humidity,
                "esp32_gaz": esp32_gaz
            },
            "devices": {
                "kitchen": kitchen,
                "living": living,
                "bedroom": bedroom,
                "cooler": cooler
            }
        })
    except Exception as e:
        logger.error(f"Eroare la obținerea statusului casei inteligente: {e}")
        return web.json_response({"error": str(e)}, status=500)

# Configurare server HTTP
app = web.Application()

# Rute pentru API
app.add_routes([
    # Rute pentru senzori
    web.get('/api/temperature', get_temperature),
    web.get('/api/humidity', get_humidity),
    web.get('/api/gaz', get_gaz),
    web.get('/api/temperature/history', get_temperature_history),
    web.get('/api/humidity/history', get_humidity_history),
    web.get('/api/gaz/history', get_gaz_history),
 web.get('/api/{sensor}/stats', get_sensor_stats),         # /api/temperature/stats
    web.get('/api/cooler/usage', get_cooler_usage),           # /api/cooler/usage
    
    # Rute pentru dispozitive
    web.get('/api/devices/{device}', get_device_state),
    web.post('/api/devices/{device}', set_device_state),
    
    # Rută pentru status complet
    web.get('/api/status', get_smart_home_status),
])

if __name__ == "__main__":
    logger.info("🚀 Server proxy HTTP rulând pe port 8080")
    web.run_app(app, host="0.0.0.0", port=8080)