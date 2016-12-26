from tkinter import *
import serial
import threading #used to make sure the UI and serial are independent

class App:

    def __init__(self, master, ser):
        self.ser = ser

        self.stateStr = StringVar()
        self.otStr = StringVar()
        self.opStr = StringVar()
        self.egtStr = StringVar()
        self.rpmStr = StringVar()
        self.thrStr = StringVar()

        self.label = Label(master, textvariable=self.stateStr)
        self.label.place(x=10,y=10)
	
        self.label = Label(master, textvariable=self.otStr)
        self.label.place(x=10,y=30)

        self.label = Label(master, textvariable=self.opStr)
        self.label.place(x=10,y=50)

        self.label = Label(master, textvariable=self.egtStr)
        self.label.place(x=10,y=70)

        self.label = Label(master, textvariable=self.rpmStr)
        self.label.place(x=10,y=90)

        self.label = Label(master, textvariable=self.thrStr)
        self.label.place(x=10,y=110)

        self.start = Button(master, text='START', width='10', height='5', bg='green', command=self.start)
        self.start.place(x=0,y=130)

        self.stop = Button(master, text='STOP', width='10', height='5', bg='red', command=self.stop)
        self.stop.place(x=140,y=130)

    def updateLabels(self, state, ot, op, egt, rpm, thr):
        self.stateStr.set('State: ' + state)
        self.otStr.set('Oil Temperature: ' + ot)
        self.opStr.set('Oil Pressure: ' + op)
        self.egtStr.set('Exhaust Gas Temperature: ' + egt)
        self.rpmStr.set('RPM: ' + rpm)
        self.thrStr.set('Thrust: ' + thr)

    def update(self): #, state, ot, op, egt, rpm, thr ):
        self.updateLabels('state', 'ot', 'op', 'egt', 'rpm', 'thr')

    def start(self):
        print('START')
        self.ser.write(b'start')

    def stop(self):
        print('STOP')
        self.ser.write(b'stop')

    def processInput(self):
        hashtag = self.ser.readline()
        
        if hashtag == b'#\r\n':
            stateStr = self.ser.readline()
            otStr = self.ser.readline()
            opStr = self.ser.readline()
            egtStr = self.ser.readline()
            rpmStr = self.ser.readline()
            thrStr = self.ser.readline()

            print(stateStr + otStr + opStr + egtStr + rpmStr + thrStr)

            if False: # otStr == b'' | opStr == b'' | egtStr == b'' | rpmStr == b'' | thrStr == b'' | otStr == b'#' | opStr == b'#' | egtStr == b'#' | rpmStr == b'#' | thrStr == b'#':
                return
            else:
                self.updateLabels( stateStr.decode('utf-8'), otStr.decode('utf-8'), 
                    opStr.decode('utf-8'), egtStr.decode('utf-8'), 
                    rpmStr.decode('utf-8'), thrStr.decode('utf-8') )
        else:
            return

def UI():
    master = Tk() #creates instace of tk window
    ser = serial.Serial('/dev/ttyUSB0', 9600, timeout=0.5)
    app = App(master, ser) #creates instance of the app class that I use for all the GUI components
    master.geometry('250x250+10+10') #defines the geometry of the tk window

    #I use this to replace the mainloop() command since that is blocking
    #I need a non-blocking version that I can control so I can monitor the serial inputs
    while True: 
        master.update_idletasks()
        master.update()
        app.processInput()

def main():
    appThread = UI()
    appThread.start()

#calls the main() method at program start
#this can be done if you just like the idea of scripting it, but this is easier to read and edit if the program gets bigger
if  __name__ =='__main__':main()
