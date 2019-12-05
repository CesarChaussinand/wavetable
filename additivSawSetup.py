import math

fichier = open("additivSaw.wt","w")

#canal gauche

for x in range(64):
	tri =""
	for i in range(128):
		valeur = 0
		for j in range(x):
			if (j%2!=0):
				valeur += math.sin((i/128.0)*2*3.1416*(j+1))/(j+1)
		tri = tri + str(valeur/2.0) + "\n"
	fichier.write(tri)

#canal droit

for x in range(64):
	tri =""
	for i in range(128):
		valeur = 0
		for j in range(x):
			if (j%2!=0):
				valeur += math.sin((i/128.0)*2*3.1416*(j+1))/(j+1)
		tri = tri + str(valeur/2.0) + "\n"
	fichier.write(tri)
	
#_________________________________#
fichier.close()
print "wave ok"

quit()
