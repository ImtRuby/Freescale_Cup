#import matplotlib
#matplotlib.use('TkAgg')

#from numpy import arange, sin, pi
#from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg, NavigationToolbar2TkAgg

import tkinter as Tk
import random
import sys
from threading import Thread
import time

#from matplotlib.figure import Figure
from Frames import *
from Model import Model

class Application(Tk.Frame):
        
    def __init__(self, **kwargs):
        #Create window
        self.root = Tk.Tk()

        #Init master frame
        Tk.Frame.__init__(self,self.root,width=640, height=480)
        self.pack()

        #Create Model
        self.model = Model()

        #COM Frame
        self.frame_com_ports = COM_Frame(self,self.model,bd=2,relief=Tk.GROOVE)
        self.frame_com_ports.grid(column=0,row=0,sticky='N',pady=5,padx=5)

        #Logger frame
        self.frame_logger = Logger_Frame(self,self.model,bd=2,relief=Tk.GROOVE)
        self.frame_logger.grid(column=0,row=1,sticky='N',pady=5,padx=5)

        #Graph frame
        self.frame_graph1 = Graph_Frame(self,self.model,bd=2,relief=Tk.GROOVE)
        self.frame_graph1.grid(column=1,row=0,sticky='EW',pady=5,padx=5,rowspan=2)

        #Quit button
        self.bouton_quitter = Tk.Button(self, text="x",command = self.stop)
        self.bouton_quitter.grid(column=2,row=0,sticky='N')
        
        """self.f = Figure(figsize=(4,3), dpi=100)
        self.a = self.f.add_subplot(111)
        self.t = arange(0.0,3.0,0.01)
        self.s = sin(2*pi*self.t)
        self.a.plot(self.t,self.s)

        #plot sur canvas
        self.canvas = FigureCanvasTkAgg(self.f, master=fenetre)
        self.canvas.show()
        self.canvas.get_tk_widget().pack(side=Tk.TOP, fill=Tk.BOTH, expand=1)
        """          


    def stop(self):
        self.model.stop()
        
