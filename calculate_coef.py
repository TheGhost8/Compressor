import pandas as pd
data = pd.read_csv('results.csv')
raw_data = 0
ari_compressed_data = 0
ppm_compressed_data = 0
for i in range(8):
	raw_data += data['original_size'][i]
	ari_compressed_data += data['compressed_size'][i]
	ppm_compressed_data += data['compressed_size'][i+8]
file_2 = data['compressed_size'][1]/data['original_size'][1]
file_3 = data['compressed_size'][2]/data['original_size'][2]
print(data)
print("ariphmetic compression coeficient: ", "%.3f" % (ari_compressed_data/raw_data))
print("ariphmetic compression on 2 & 3 files: ", "%.3f" % ((file_2+file_3)/2))
file_2 = data['compressed_size'][8]/data['original_size'][8]
file_3 = data['compressed_size'][9]/data['original_size'][9]
print("ppm compression coeficient: ", "%.3f" % (ppm_compressed_data/raw_data))
print("ppm compression on 2 & 3 files: ", "%.3f" % ((file_2+file_3)/2))