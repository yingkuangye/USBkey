import serial
import sys
import os
import time
import re

import serial.tools.list_ports

global MAX_LOOP_NUM
global newCmd

MAX_LOOP_NUM = 10


def waitForCmdOKRsp(serInstance):
    maxloopNum = 0
    data_received = ""
    while True:

        line = serInstance.readline()                              #读取一行数据
        maxloopNum = maxloopNum + 1                        #计算读取长度
        
        try:
            data_received = line.decode('utf-8')
            #print("Rsponse_1 : %s" % line.decode('utf-8'))
            print("Rsponse_2 : %s" % data_received)  #串口接收到数据，然后显示
        except:
            print("exception")
            pass

        if (re.search(b'OK', line)):
            break
        elif (maxloopNum > MAX_LOOP_NUM):
            print("access denied")
            sys.exit(0)

        if data_received == "test\n":   #readline函数会将换行符读进来，所以记得加上\n
           break

    return data_received


def sendAT_Cmd(serInstance, atCmdStr, waitforOk):
   # print("Command: %s" % atCmdStr)
    serInstance.write(atCmdStr.encode('utf-8'))   # atCmdStr  波特率
    # or define b'string',bytes should be used not str

    if (waitforOk == 1):
        data_received = waitForCmdOKRsp(serInstance)
       # print("password = %s" % data_received)
    else:
        #waitForCmdRsp()
        waitForCmdOKRsp(serInstance)
    return data_received



#该代码刚开始接收到的几次数据都是有问题的，但是到后面就还好