# -*- coding: utf-8 -*-
import requests, random, time, configparser


def getRequestId():
    return str(random.randint(100000, 999999))


if __name__ == "__main__":
    # preference variable
    config = configparser.ConfigParser()
    config.read('config.ini')
    url = "http://sandbox.sktiot.com:9000"
    node_id = config.get('thingplug', 'node_ID')
    container_name = config.get('thingplug', 'container_name')
    mgmtCmd_prefix = config.get('thingplug', 'mgmtCmd_prefix')
    app_id = config.get('thingplug', 'app_ID')
    uKey = config.get('thingplug', 'uKey')

    # 1. contentInstance를 활요한 서버에 저장된 센서 값 조회(Retrieve)
    print("### 1. retrieve latest content Instance ###")
    url_path = "/ThingPlug/remoteCSE-" + node_id + "/container-" + container_name + "/latest"
    header = {
        'X-M2M-Origin': app_id,
        'X-M2M-RI': getRequestId(),
        'Accept': 'application/json',
        'locale': 'ko',
        'uKey': uKey
    }

    body = {}

    res = requests.get(url + url_path, headers=header, json=body)

    if res.status_code == 400:
        print("Retrieve Error")
        exit()

    res_json = res.json()
    if "con" in res_json:
        print("content : " + res_json["con"])
    if "ri" in res_json:
        print("resource id : " + res_json["ri"])
    if "ct" in res_json:
        print("date : " + res_json["ct"])

    # 2. mgmtCmd 요청
    print("### 2. mgmtCmd 제어 요청 ###")
    url_path = "/ThingPlug/mgmtCmd-" + mgmtCmd_prefix + node_id
    header = {
        'X-M2M-Origin': app_id,
        'X-M2M-RI': getRequestId(),
        'Accept': 'application/json',
        'Contnet-Type': 'application/json',
        'uKey': uKey
    }

    body = {
        'exra': 'testArgument',
        'exe': 'true'
    }

    res = requests.put(url + url_path, headers=header, json=body)

    if res.status_code != 200:
        print("Sent mgmtCmd Error")
        exit()

    res_json = res.json()
    if "exin" in res_json:
        exin = res_json['exin']
        if "ri" in exin[0]:
            ri = str(exin[0]['ri'])
            print("resourceID : " + ri)
        if "exs" in exin[0]:
            print("execStatus : " + str(exin[0]['exs']))

    # 3. execInstance 리소스 조회
    print("### 3. execInstance 리소스 조회 ###")
    time.sleep(2)

    url_path = "/ThingPlug/mgmtCmd-" + mgmtCmd_prefix + node_id + "/execInstance-" + ri
    header = {
        'X-M2M-Origin': app_id,
        'X-M2M-RI': getRequestId(),
        'Accept': 'application/json',
        'locale': 'ko',
        'uKey': uKey
    }

    body = {}

    res = requests.get(url + url_path, headers=header, json=body)

    if res.status_code != 200:
        print("retrieve execInstance error : " + str(res.status_code))
        exit()

    res_json = res.json()
    if "ri" in res.json():
        ri = res_json['ri']
        print["resourceID : " + str(res_json['ri'])]
    if " exs" in res_json:
        print("execStatus : " + str(res_json['exs']))
