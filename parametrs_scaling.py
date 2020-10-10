import sys

frequency, aggressiveness = sys.argv[1], sys.argv[2]

file = open("src/parametrs.h", "w")
file.write("#pragma once\n")
file.write("\n")
s = "#define MAX_FREQUENCY " + str(frequency) + "\n"
file.write(s)
s = "#define AGGRESIVENESS " + str(aggressiveness)
file.write(s)
file.close()