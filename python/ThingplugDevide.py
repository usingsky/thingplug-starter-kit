# -*- coding: utf-8 -*-

import requests, random, time, threading, configparser
import paho.mqtt.client as mqtt
import xml.etree.ElementTree as ET

def getRequestId():
    return str(random.randint(100000, 999999))

def mqtt_on_connect(client, userdata, flags, response_code):
    print("mqtt_on_connect : "+str(response_code))
    client.subscribe(str(mqtt_topic))

def mqtt_on_message(client, userdata, msg):
    print("### MQTT Message 수신")
    print("mqtt_on_message : "+str(msg.payload))

    msg_code = str(msg.payload).replace("b'", "").replace("'", "")
    doc = ET.XML(msg_code)

    for elem in doc.iterfind("pc/exin"):
        for child in elem.iterfind("exra"):
            mqtt_extra = child.text
            print("extra : "+mqtt_extra)

    for elem in doc.iterfind("pc/exin/ri"):
        url_path = "/ThingPlug/mgmtCmd-"+mgmtCmd_prefix+node_id+"/execInstance-"+elem.text
        print(url_path)
        header = {
            'X-M2M-Origin': node_id,
            'X-M2M-RI': getRequestId(),
            'X-M2M-NM': node_id,
            'Accept': 'application/json',
            'Content-Type': 'application/json',
            'locale': 'ko',
            'dKey': dkey
        }

        body = {}

        res = requests.put(url + url_path, headers=header, json=body)

        if res.status_code != 200:
            print("update execInstance error : " + str(res.status_code))
            exit()

        res_json = res.json()
        if "ri" in res_json:
            print("처리한 resourceID : "+str(res_json["ri"]))
        if "exs" in res_json:
            print("처리한 결과 execStatus : "+str(res_json["exs"]))

    print("### MQTT Message 수신 End ###")

def setContentInterval(interval):
    while True:
        url_internal_path = "/ThingPlug/remoteCSE-"+node_id+"/container-"+container_name
        internal_header = {
            'X-M2M-Origin': node_id,
            'X-M2M-RI': getRequestId(),
            'dkey': dkey,
            'locale': 'ko',
            'Accept': 'application/json',
            'Content-Type': 'application/json;ty=4',
        }

        internal_body = {
            'cnf': "text",
            'con': getRequestId()
        }

        internal_res = requests.post(url + url_internal_path, headers=internal_header, json=internal_body)

        if internal_res.status_code >= 400:
            print("sent content error")
            exit()

        internal_res_json = internal_res.json()
        print(str(internal_res_json))
        if "con" in internal_res_json and "ri" in internal_res_json:
            print("send data - content : "+internal_res_json["con"]+", resourceID : "+internal_res_json["ri"])

        time.sleep(interval)


if __name__ == "__main__":
    # preference variable
    config = configparser.ConfigParser()
    config.read('config.ini')
    url = "http://sandbox.sktiot.com:9000"
    node_id = config.get('thingplug', 'node_ID')
    pass_code = config.get('thingplug', 'passCode')
    container_name = config.get('thingplug', 'container_name')
    mgmtCmd_prefix = config.get('thingplug', 'mgmtCmd_prefix')
    cmdType = config.get('thingplug', 'cmdType')
    mqtt_topic = "oneM2M/req/+/"+node_id

    # 1. node 생성
    print ("### 1. node 생성 ###")
    url_path = "/ThingPlug"

    header = {
        'X-M2M-Origin': node_id,
        'X-M2M-RI': getRequestId(),
        'X-M2M-NM': node_id,
        'Accept': 'application/json',
        'Content-Type': 'application/json;ty=14'
    }

    body = {
        "ni": node_id
    }

    res = requests.post(url+url_path, headers=header, json=body)

    if res.status_code == 409 :
        print("이미 생성된 node resource ID 입니다.")
    elif res.status_code == 404 :
        print("잘못된 URL")
        exit()

    res_json = res.json()
    if "ri" in res_json:
        ri = res_json["ri"]
        print("node resource id : "+ri)
    else:
        print("ERROR")
        exit()

    # 2. remoteCSE 생성
    print("### 2. remoteCSE 생성 ###")
    url_path = "/ThingPlug"

    header = {
        'X-M2M-Origin': node_id,
        'X-M2M-RI': getRequestId(),
        'X-M2M-NM': node_id,
        'passCode': pass_code,
        'Accept': 'application/json',
        'Content-Type': 'application/json;ty=16'
    }

    body = {
        "cst": 3,
        "csi": node_id,
        "poa": ["MQTT|"+node_id],
        "rr": "true",
        "nl": ri
    }

    res = requests.post(url + url_path, headers=header, json=body)

    if res.status_code == 409:
        print("이미 생성된 remoteCSE 입니다.")

    res_header = res.headers
    if "dkey" in res_header:
        dkey = res_header["dkey"]
        print("디바이스 키 : "+dkey)
    else:
        print("ERROR")
        exit()

    if "content-location" in res_header:
        print("content-location : "+res_header["content-location"])

    # 3. container 생성
    print("### 3. contatiner 생성 ###")
    url_path = "/ThingPlug/remoteCSE-"+node_id

    header = {
        'X-M2M-Origin': node_id,
        'X-M2M-RI': getRequestId(),
        'X-M2M-NM': container_name,
        'dkey': dkey,
        'locale':'ko',
        'Accept': 'application/json',
        'Content-Type': 'application/json;ty=3'
    }

    body = {
        "containerType": "heartbeat",
        "heartbeatPeriod": 300
    }

    res = requests.post(url + url_path, headers=header, json=body)

    if res.status_code == 409:
        print("이미 생성된 container 입니다.")

    res_header = res.headers
    if "content-location" in res_header:
        print("content-location : " + res_header["content-location"])
    else:
        print("ERROR")
        exit()

    # 4. mgmtCmd 생성
    print("### 4. mgmtCmd 생성 ###")
    url_path = "/ThingPlug"

    header = {
        'X-M2M-Origin': node_id,
        'X-M2M-RI': getRequestId(),
        'X-M2M-NM': mgmtCmd_prefix+node_id,
        'dkey': dkey,
        'locale': 'ko',
        'Accept': 'application/json',
        'Content-Type': 'application/json;ty=12'
    }

    body = {
        "cmt": cmdType,
        "exe": "true",
        "ext": ri
    }

    res = requests.post(url + url_path, headers=header, json=body)

    if res.status_code == 409:
        print("이미 생성된 mgmtCmd 입니다.")

    res_header = res.headers
    if "content-location" in res_header:
        print("content-location : " + res_header["content-location"])
    else:
        print("ERROR")
        exit()

    # 5. content instance 생성
    print("### 5. content instance 생성 ###")
    thread = threading.Thread(target=setContentInterval, args=(2,))
    thread.start()

    # 6. MQTT Connect
    print("### 6. MQTT Connect(optional) ###")
    mqtt_client = mqtt.Client(protocol=mqtt.MQTTv311)
    mqtt_client.on_connect = mqtt_on_connect
    mqtt_client.on_message = mqtt_on_message

    mqtt_client.connect(host="sandbox.sktiot.com", port=1883, keepalive=60)
    mqtt_client.loop_forever()



