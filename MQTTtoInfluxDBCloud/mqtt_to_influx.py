#! /usr/bin/python3
#
#	subscribe_influx.py
#
#						Nov/05/2022
#
# ------------------------------------------------------------------
import sys
import json
from time import sleep
import paho.mqtt.client as mqtt
import time
from influxdb_client import InfluxDBClient,Point ,WritePrecision
from influxdb_client.client.write_api import SYNCHRONOUS
from point import pointArr

# ------------------------------------------------------------------
def influxdb_write_proc(json_str):
        lat = 34.000000
        lon = 136.00000
        print(json_str)
        json_dict = json.loads(json_str)
        sensorID = json_dict["ID"]
        for item in pointArr:
            if item[0] == int(sensorID):
                lat =item[1]
                lon =item[2]
                break
        print(f"sensorID:{sensorID} lat:{lat} lon:{lon}")
        json_dict["lat"] = float(lat)
        json_dict["lon"] = float(lon)
        dict_structure ={"measurement":json_dict.pop("sensor")}
        dict_structure["tags"]={"ID":json_dict.pop("ID")}
        dict_structure["fields"]=json_dict
        dict_structure["time"]=int(time.time())
        # print(dict_structure)
        pp = Point.from_dict(dict_structure,WritePrecision.S)
        write_api.write(bucket=bucket,org=org ,record=pp)
        # print(pp)
# ------------------------------------------------------------------
def on_connect(client, userdata, flags, respons_code):
#	print('status {0}'.format(respons_code))
	client.subscribe(topic)

# ------------------------------------------------------------------
def on_message(client, userdata, msg):
#	print(msg.topic + ' ' + str(msg.payload,'utf-8'))
	influxdb_write_proc(str(msg.payload,'utf-8'))

# ------------------------------------------------------------------
sys.stderr.write("*** start ***\n")

host = 'localhost'
port = 1883
topic = 'LTE/+'

# token = os.environ.get("INFLUXDB_TOKEN")
token = "***INFLUX API TOKEN***"
org = "***INFLUX ORG***"
url = "https://us-east-1-1.aws.cloud2.influxdata.com"
bucket="lorasensor"
db_client = InfluxDBClient(
   url=url,
   token=token,
   org=org
)

write_api = db_client.write_api(write_options=SYNCHRONOUS)

client = mqtt.Client(protocol=mqtt.MQTTv311)

client.on_connect = on_connect
client.on_message = on_message

client.connect(host, port=port, keepalive=60)

client.loop_forever()

sys.stderr.write("*** end ***\n")
# ------------------------------------------------------------------
