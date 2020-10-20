import pandas as pd
data = pd.read_csv('results.csv')
raw_data = 0
ari_compressed_data = 0
ppm_compressed_data = 0
for i in range(8):
	raw_data += data['original_size'][i]
	ari_compressed_data += data['compressed_size'][i]
	ppm_compressed_data += data['compressed_size'][i+8]
print(data)
print("ariphmetic compression coeficient: ", "%.3f" % (ari_compressed_data/raw_data))
print("ariphmetic compression on 2 & 3 files: ", "%.3f" % ((data['compressed_size'][1] + data['compressed_size'][2])/(data['original_size'][1] + data['original_size'][2])))
print("ppm compression coeficient: ", "%.3f" % (ppm_compressed_data/raw_data))
print("ariphmetic compression on 2 & 3 files: ", "%.3f" % ((data['compressed_size'][9] + data['compressed_size'][10])/(data['original_size'][9] + data['original_size'][10])))