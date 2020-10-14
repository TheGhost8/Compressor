import sys
import pandas as pd
data = pd.read_csv('results.csv')
raw_data = 0
ari_compressed_data = 0
for i in range(8):
	raw_data += data['original_size'][i]
	ari_compressed_data += data['compressed_size'][i]

file = open("ari_parametrs_statistic.csv", "a")
frequency, aggressiveness = sys.argv[1], sys.argv[2]

file_2 = data['compressed_size'][1] / data['original_size'][1]
file_3 = data['compressed_size'][2] / data['original_size'][2]

file.write("\n")
s = str(frequency) + ","
s += str(aggressiveness) + ","
s += ("%.4f" % (ari_compressed_data/raw_data)) + ","
s += ("%.4f" % (file_2 + file_3 / 2)) + ","
s += ("%.4f" % file_2) + ","
s += ("%.4f" % file_3)
file.write(s)
file.close()