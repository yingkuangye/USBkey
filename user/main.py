# serial_rxd.py
import time
import sys
sys.path.append('/home/yingkuangye/.local/lib/python3.8/site-packages')
import serial
import _thread
import io

import ctypes
mylib = ctypes.cdll.LoadLibrary('./libdilithium2_clean.so')


data_ser = serial.Serial("/dev/ttyUSB0",115200,timeout = 5)
data_ser.flushInput()
#sio = io.TextIOWrapper(io.BufferedRWPair(data_ser, data_ser),newline=None,
    #                   errors=None,encoding=None)
sm = b''
pk = b''

def get_data():
    global sm
    global pk
    data_ser.write(b'100000000\r') 
    while True:
        recv = data_ser.read_until()
        print(" --- data_recv --> ", recv)  
        if(recv == b'pk\n'):
            sm = b''
            pk = b''
            print("reading pk")
            recv = data_ser.read_until()
            print(" --- pk_1 --> ", recv)  
            while(recv != b"sm\n"):
                pk = pk + recv
                recv = data_ser.read_until()
                print(" --- pk --> ", recv)  
                print("reading pk") 
            recv = data_ser.read_until()
            while(recv != b"transmit done\n"):
                sm = sm + recv
                recv = data_ser.read_until()
                print(" --- sm--> ", recv)  
                print("reading sm") 
            recv = data_ser.read_until()   
        print("all done")
        print("sm = ",sm)
        print("pk = ",pk)        


def get_orignal_data():
    while True:
      data_ser.write(b'100000000\r') 
      recv = data_ser.read_until()
      print(" --- data_recv --> ", recv) 


if __name__ == '__main__':

    _thread.start_new_thread(get_data()) # 开启线程，执行get_data方法
    while 1:
        time.sleep(1)
        #print(len(message))
        #data_ser.write('100000000\r\n') # 发送二进制0
        
        
        #mlen = ctypes.c_size_t()
        #smlen = ctypes.c_size_t()
        #print("pk = ", pk[:-1])
        #print("sm = ",sm[:-1])
        #result = mylib.PQCLEAN_DILITHIUM2_CLEAN_crypto_sign_open(sm[8:-1], ctypes.byref(mlen), sm[8:-1], smlen, pk[8:-1])
        #print(result)
