# -*- coding: utf-8 -*-
"""
Created on Thu Jul 16 06:40:31 2020

Class for Nodal Encounter Patterns, their import and handling

@author: DESAUCHR
"""

import numpy as np #for array representation of the NEP
from tkinter import * #required for closing dialog windown
from tkinter import filedialog #UI dialog window to select import file

class NEP:
    #The NEP initializes with default values and array initializations
    def __init__(self):
        self.importFileName = ""
        self.NEP = np.zeros([10, 10, 10])
        self.T = np.zeros([10])
        self.dt = 0.0
        self.initialized = False
        self.numberOfNodes = 10;
        
    #The import file must be attached, before parsing is attempted
    def attachImportFile(self, fileName = "None"):
        #A file selection dialog is used, if no file name was selected
        if(fileName == "None"):
            window = Tk()
            fileName = filedialog.askopenfilename(initialdir="/NEP_import/", title="Choose .csv file for NEP import")
            window.destroy()
        self.importFileName = fileName
        #The file name must contain the dt information in the format *dt=XX.XXs*
        self.dt = float(fileName.split("dt=")[1].split("s")[0])
        
    #Parsing is the process of importing the encounter data from a csv file, to the class object
    def parse(self):
        #it is checked if a valid file was attached
        if((".mat" in self.importFileName) | (self.dt == 0)):
            return False
        
        #These are the standard delimiters
        delim1 = "," #delimiter between connection information from he same node
        delim2 = "|" #delimiter between connection information of different nodes
        delim3 = "\n" #delimiter between time steps
        
        #Open the file and import the data as a string
        file = open(self.importFileName,"r")
        data = file.read()
        file.close()
        
        #read the time duration of the recorded NEP
        timeDuration = data.count(delim3) + 1
        #generate a time vector
        self.T = np.zeros([timeDuration])
        for t in range(timeDuration):
            self.T[t] = t * self.dt
        #extract the number of nodes in the recording
        nodeNumber = int((data.count(delim2) + timeDuration)/timeDuration)
        self.numberOfNodes = nodeNumber
        
        #build the default NEP
        self.NEP = np.zeros([nodeNumber,nodeNumber,timeDuration]);

        #deconstruct the data from the file and fill the NEP
        t = 0
        for NEPentry in data.split(delim3):
            x = 0
            for recvVector in NEPentry.split(delim2):
                y = 0
                for recvState in recvVector.split(delim1):
                    self.NEP[x,y,t] = int(recvState)
                    y += 1
                x += 1
            t += 1
        
        #Parsing was successfull
        self.initialized = True
        return True
    
    #calculate the network connectedness for the complete NEP or a specific time
    def networkConnectedness(self, time = "none"):
        #get some system parameters
        NoN = self.numberOfNodes
        NoE = np.zeros(len(self.NEP[0,0]))
        #if the time is defined, calculate the NC of this time and return it
        if(time != "none"):
            buffer = buffer = sum(sum(self.NEP[:,:,time]))
            buffer = (buffer-NoN)/(NoN*NoN-NoN)
            return buffer
        #if it is not defined calculate NC for all times and return vector
        else:
            for t in range(len(self.NEP[0,0])):
                buffer = sum(sum(self.NEP[:,:,t]))
                buffer = (buffer-NoN)/(NoN*NoN-NoN)
                NoE[t] = buffer
            return NoE
    #calculate the percentage of undirectional connections for a given NEP entry
    def unidirectionalPercentage(self, c):
        n = len(c)
        nb = 0
        nc = 0
        for x in range(n):
            for y in range(n):
                #unidirectionality is connected, if x can connect to y, but y not to x
                if(c[x,y] == 1 & x != y):
                    nc += 1
                    if(c[y,x] != 1):
                        nb += 1
        if(nc > 0):
            return nb/nc
        else:
            return 1
    #returns the percentage of unidirectional connections for the complte NEP or one specific time
    def unidirectionalConnections(self, time = "none"):
        T = len(self.NEP[0,0])
        BC = np.zeros(T)
        #either calculate it for all timeslots or for one specific time
        if(time == "none"):
            for t in range(T):
                BC[t] = self.unidirectionalPercentage(self.NEP[:,:,t])
            return BC
        else:
            return self.unidirectionalPercentage(self.NEP[:,:,time])
        
    #rlt for a single route
    def rlt(self, r):
        state = False #disconnected as default
        trueSince = 0 #duration of current connection
        rlts = []
        if(r[0] == 1):
            state = True
        
        for connectionState in r[1:len(r)]:
            if(connectionState == 1):
                state = True
                trueSince += 1
            elif(state == True):
                state = False
                rlts.append(trueSince * self.dt)
                trueSince = 0
        return rlts
        
    #calculate the route life times in the NEP, return a list of rlts of connections in the NEP
    def routeLifeTime(self, transmitter = "none", receiver = "none"):
        if((transmitter == "none") | (receiver == "none")):
            rlts = []
            for x in range(self.numberOfNodes):
                for y in range(self.numberOfNodes):
                    rlts = np.append(rlts, self.rlt(self.NEP[x, y, :]))
            return rlts
        else:
            return self.rlt(self.NEP[transmitter, receiver, :])
