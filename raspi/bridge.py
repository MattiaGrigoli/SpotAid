from lora_e220 import LoRaE220, print_configuration, Configuration
#from lora_e220_operation_constant import ResponseStatusCode, AirDataRate, UARTBaudRate, TransmissionPower
from lora_e220_constants import UARTParity, UARTBaudRate, TransmissionPower, FixedTransmission, AirDataRate, \
    OperatingFrequency, LbtEnableByte, WorPeriod, RssiEnableByte, RssiAmbientNoiseEnable, SubPacketSetting
from lora_e220_operation_constant import ResponseStatusCode, ModeType, ProgramCommand, SerialUARTBaudRate, \
    PacketLength, RegisterAddress
import serial
import json
import time
from datetime import datetime
import asyncio
import uvicorn
from fastapi import FastAPI, Request
from pydantic import BaseModel
from typing import List
import httpx
import struct

from rest_framework.fields import DateTimeField

app = FastAPI()
loraSerial = serial.Serial('/dev/ttyAMA0', baudrate=9600)
lora = LoRaE220('900T22D', loraSerial, aux_pin=4, m0_pin=23, m1_pin=24)

class Product(BaseModel):
    id: int = 0
    name: str = ""
    price: float = 0.0
    quantity: int = 0


class ProductInside(BaseModel):
    id: int
    id_distributor: int
    id_product: int
    quantity: int

class Distributor(BaseModel):
    id: int
    status: str
    address: str
    position_x: float
    position_y: float

class Selling(BaseModel):
    date_time: datetime
    id_distributor: int
    id_product: int

async def getDetails(id: int):
    url = f"http://192.168.1.82:8000/ServerApplication/api/product/{id}/"
    async with httpx.AsyncClient() as client:
        try:
            response = await client.get(url)
            if response.status_code == 200:
                json = response.json()
                product = Product(**json)
                print(product.id, product.name, product.price)
                return product
            else:
                return None
        except Exception as e:
            print(f"error:{type(e).__name__} - {e}")
            return None

@app.post("/")
async def receive_list(items: List[ProductInside]):
    payloads = []
    id_distributor = 0
    for item in items:
        print(f"Distributore: {item.id_distributor} - Prodotto: {item.id_product} - Qty: {item.quantity}")
        product = await getDetails(item.id_product)
        id_distributor = item.id_distributor
        if product:
            product.quantity = item.quantity
            p_string = f"{product.id},{product.name},{product.price:.2f},{item.quantity}"
            payloads.append(p_string)
    if payloads:
        final_message = ";".join(payloads)
        await send_lora(id_distributor, final_message)

    return {"status": "success"}

# To add a selling to the database
async def addSelling(id_d: int, id_p: int):
    new_selling = Selling(date_time = datetime.now(), id_distributor = id_d, id_product = id_p)

    url = f"http://192.168.1.82:8000/ServerApplication/api/selling/"
    async with httpx.AsyncClient() as client:
        try:
            response = await client.post(url, json = json.loads(new_selling.model_dump_json()))
            if response.status_code == 200:
                return response
        except Exception as e:
            print(f"error:{type(e).__name__} - {e}")
    return None

async def send_lora(id: int, payload):
    print("seding to", id, "payload:", payload)
    lora.send_fixed_message(0, id, 23, payload)
    return

async def lora_listener():
    code = lora.begin()
    print("Initialization: {}", ResponseStatusCode.get_description(code))
    while True:
        # listen LoRa
        await asyncio.sleep(1)

async def main():
    config = uvicorn.Config(app, host="0.0.0.0", port=8000)
    server = uvicorn.Server(config)
    await asyncio.gather(
        lora_listener(),
        server.serve()
    )

if __name__ == '__main__':
    asyncio.run(main())

#time.sleep(0.5)

#code, configuration = lora.get_configuration()
#print(code)
#print(configuration)
#print("Retrieve configuration: {}", ResponseStatusCode.get_description(code))
#if configuration is None:
	#print("errore di configurazione")
#else:
	#print_configuration(configuration)

# configuration_to_set = Configuration('900T22D')
# configuration_to_set.ADDL = 0x02
# configuration_to_set.ADDH = 0x00
# configuration_to_set.CHAN = 32

# configuration_to_set.SPED.airDataRate = AirDataRate.AIR_DATA_RATE_010_24
# #configuration_to_set.SPED.uartParity = UARTParity.MODE_00_8N1
# configuration_to_set.SPED.uartBaudRate = UARTBaudRate.BPS_9600

# configuration_to_set.OPTION.transmissionPower = TransmissionPower('900T22D').\
#                                                     get_transmission_power().POWER_22
# # or
# # configuration_to_set.OPTION.transmissionPower = TransmissionPower22.POWER_10

# #configuration_to_set.OPTION.RSSIAmbientNoise = RssiAmbientNoiseEnable.RSSI_AMBIENT_NOISE_ENABLED
# #configuration_to_set.OPTION.subPacketSetting = SubPacketSetting.SPS_064_10

# #configuration_to_set.TRANSMISSION_MODE.fixedTransmission = FixedTransmission.FIXED_TRANSMISSION
# #configuration_to_set.TRANSMISSION_MODE.WORPeriod = WorPeriod.WOR_1500_010
# #configuration_to_set.TRANSMISSION_MODE.enableLBT = LbtEnableByte.LBT_DISABLED
# #configuration_to_set.TRANSMISSION_MODE.enableRSSI = RssiEnableByte.RSSI_ENABLED

# #configuration_to_set.CRYPT.CRYPT_H = 1
# #configuration_to_set.CRYPT.CRYPT_L = 1


# # Set the new configuration on the LoRa module and print the updated configuration to the console
# code, confSetted = lora.set_configuration(configuration_to_set)
