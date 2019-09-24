import math

sin = ""
for i in range(128):
	sin = sin + str(math.sin(i*2*math.pi/128)) + "\n"
	
saw = ""
for i in range(128):
	saw = saw + str((i/64.0)-1.0) + "\n"
	
sqr = ""
for i in range(128):
	sqr = sqr + str(((i<64)*2)-1) + "\n"
	
isaw = ""
for i in range(128):
	if i<64:
		isaw = isaw + str(i/64.0) + "\n"
	else:
		isaw = isaw + str(-(128-i)/64.0) + "\n"
		
fichier = open("wave","w")
fichier.write(sin)
fichier.write(saw)
fichier.write(sqr)
fichier.write(isaw)
fichier.close()
print "wave ok"

quit()
