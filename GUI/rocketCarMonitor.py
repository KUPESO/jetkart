from tkinter import *
import serial
import threading #used to make sure the UI and serial are independent

class App:

    def __init__(self, master):
        self.s1 = StringVar()

        self.label = Label(master, textvariable=self.s1)
        self.label.place(x=10,y=10)

        self.update = Button(master, text="Update", command=self.update)
        self.update.place(x=10,y=50)

    def update(self): #, state, ot, op, egt, rpm, thr ):
        self.s1.set("Updated")

def UI():
    master = Tk() #creates instace of tk window
    app = App(master) #creates instance of the app class that I use for all the GUI components
    master.geometry("250x250+10+10") #defines the geometry of the tk window

    ser = serial.Serial('/dev/ttyUSB0', 9600, timeout=0.1)
    #I use this to replace the mainloop() command since that is blocking
    #I need a non-blocking version that I can control so I can monitor the serial inputs
    while True: 
        print(ser.readline())
        master.update_idletasks()
        master.update()

def main():
    appThread = UI()
    appThread.start()

#calls the main() method at program start
#this can be done if you just like the idea of scripting it, but this is easier to read and edit if the program gets bigger
if  __name__ =='__main__':main()
