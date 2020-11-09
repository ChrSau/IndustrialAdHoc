# -*- coding: utf-8 -*-
"""
Created on Thu Jul 16 06:40:31 2020

@author: DESAUCHR
"""

#import the NEP class
from nep import NEP
#plotting class for visualization
from matplotlib import pyplot as plt

#initialize empty NEP
cNEP = NEP()
#attach file to the NEP
cNEP.attachImportFile()
#parse the file
cNEP.parse()

#Use different processing methods
#D = cNEP.networkConnectedness()
#D = cNEP.unidirectionalConnections()
rlts = cNEP.routeLifeTime()


#normal plotting:
#plt.plot(D)
#plt.show()

#rlt plotting:
rltBins = range(0, 50)
n,bins = plt.hist(x=rlts, bins=rltBins)
for r in range(n):
    r = r / len(rlts)
plt.plot(bins,n)
plt.show()